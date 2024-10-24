#ifndef thicket_context_resolve_fin_hpp
#define thicket_context_resolve_fin_hpp

#include "context.hpp" // not necessary, just help IDE to resolve symbols

namespace cornus_thicket {

void
Context::resolveFilesystemNode(Node& n){

    if(!n.valid_ || n.resolved_ >= NODE_RESOLVED){
        return;
    }

    if(n.resolved_ == NODE_RESOLVING){
        report_error(std::string(
                "Circular dependency encountered while resolving final node at ")
                    + p2s(n.path_),
                    SEVERITY_ERROR
        );
        return;
    }

    n.has_own_content_ = true;

    switch(n.node_type){
    case FILE_NODE:
        n.has_refernces_ = false; // --T v2
        n.resolved_ = NODE_RESOLVED;
        return;
    case DIR_NODE:
        break;
    default:
        return; // shall be unreachable (n.valid_ == false)
    }

    n.resolved_ = NODE_RESOLVING;

    // resolve directory:

    bool has_ref_descendants = false;


    for (auto const& de : fs::directory_iterator{n.path_}){
        auto& p = de.path();

        if(p.empty()){
            continue; //
        }

        //bool child_has_ref_descendants = false;

        fs::path mountpoint_path;
        if(
                de.is_regular_file()
                && is_thicket_mountpoint_description(p, &mountpoint_path)
        ){
            // mountpoint case:
            Node* cn = mountpointAt(mountpoint_path);
            if(cn != nullptr){
                // resolveReferenceNode(*cn, true);  // redundant? mountpointAt() have already resolved the node
                n.children[mountpoint_path.filename()] = cn;
                has_ref_descendants = true;
            }else{
                report_error(std::string(
                        "Can not create mountpoint at ")
                            + p2s(mountpoint_path),
                            SEVERITY_ERROR
                );
            }

            continue;
        }

        if(
                de.is_regular_file()
                && is_thicket_imprint(p)
        ){
            continue;  // ignore possible imprint file
        }


        //skip possible thicket artifact:

        if(
            fs::exists(fs::symlink_status(fs::path(p.native() + mountpoint_suffix))) ||
            fs::exists(fs::symlink_status(fs::path(p.native() + imprint_suffix)))
        ){
            continue;
        }

        // final (filesystem) case:

        {
            Node* cn = existingFileAt(p);
            if(cn == nullptr){
                // ToDo: report error???
                continue;
            }

            if(cn->valid_) {
                resolveFilesystemNode(*cn);
                n.children[p.filename()] = cn; // append existing final node as child
                has_ref_descendants = has_ref_descendants || cn->has_refernces_;
            }
        }
    }

    n.has_refernces_ = has_ref_descendants;
    n.resolved_ = NODE_RESOLVED;
}



}
#endif
