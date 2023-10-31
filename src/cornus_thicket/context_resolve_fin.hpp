#ifndef thicket_context_resolve_fin_hpp
#define thicket_context_resolve_fin_hpp

#include "context.hpp" // not necessary, just help IDE to resolve symbols

namespace cornus_thicket {

bool
Context::
is_thicket_mountpoint_description(
        const fs::path& p, // input parameter (path to mountpoint description file)
        fs::path* mountpoint_path // output parameter (path to mountpoint itself)
){
    auto fn = p.filename();
    auto s = fn.native();
    size_t len = s.length();
    if(len <= MNT_SUFFIX_LENGTH){
        return false;
    }


    if(s.substr(len - MNT_SUFFIX_LENGTH, MNT_SUFFIX_LENGTH) == MNT_SUFFIX()){
        if(mountpoint_path != nullptr){
            *mountpoint_path = p;
            mountpoint_path->replace_filename(s.substr(0, len - MNT_SUFFIX_LENGTH));
        }
        return true;
    }else{
        return false;
    }
}


void
Context::resolveFinal(Node& n){

    if(!n.valid_ || n.resolved_ > NODE_UNRESOLVED){
        return;
    }

    switch(n.target_type){
    case FILE_NODE:
        n.resolved_ = NODE_RESOLVED;
        return;
    case DIR_NODE:
        break;
    default:
        return; // shall be unreachable (n.valid_ == false)
    }

    // resolve directory:


    for (auto const& de : fs::directory_iterator{n.path_}){
        auto& p = de.path();
        if(p.empty()){
            continue; //
        }

        fs::path mountpoint_path;
        if(de.is_regular_file() && is_thicket_mountpoint_description(p, &mountpoint_path)){
            Node* cn = mountpointAt(mountpoint_path);
            if(cn != nullptr){
                resolveReference(*cn);
                n.children[p.filename()] = cn;
            }// else  report error?
            continue;
        }

        if(is_thicket_mountpoint_description(p/MNT_SUFFIX(), nullptr)){
            continue; // skip possibly generated materialization of a mountpoint
        }

        Node* cn = existingFileAt(p);
        if(cn == nullptr){
            // ToDo: report error???
            continue;
        }

        resolveFinal(*cn);
        n.children[p.filename()] = cn; // append existing final node as child
    }

    n.resolved_ = NODE_RESOLVED;
}



}
#endif
