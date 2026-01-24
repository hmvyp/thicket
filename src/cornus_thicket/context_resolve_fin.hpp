#ifndef thicket_context_resolve_fin_hpp
#define thicket_context_resolve_fin_hpp

#include "context.hpp" // not necessary, just help IDE to resolve symbols

namespace cornus_thicket {

void
Context::resolveFilesystemNode(Node& n){

    struct ChildEntry{
        fs::path p;
        fs::file_status symstatus;
    };

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

    std::vector<ChildEntry> files; // to process as final nodes
    std::set<fs::path> exclude_files; // thicket artifacts to ignore in files

    for (auto const& de : fs::directory_iterator{n.path_}){
        fs_errcode ec;
        auto symstat = de.symlink_status(ec);
        auto& p = de.path();

        if( // ignore invalid, empty or special files:
                ec ||
                p.empty() ||
                !(
                        fs::is_regular_file(symstat) ||
                        fs::is_directory(symstat) ||
                        fs::is_symlink(symstat) // symlink is ok, but treated as just a file
                )
        ){
            continue; //
        }

        //bool child_has_ref_descendants = false;

        fs::path mountpoint_path;
        if(
                fs::is_regular_file(symstat)
                && is_thicket_mountpoint_description(p, &mountpoint_path)
        ){
            // mountpoint case:
            exclude_files.emplace(mountpoint_path);

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

        fs::path imprint_artifact;
        if(
                fs::is_regular_file(symstat)
                && is_thicket_imprint(p, &imprint_artifact)
        ){
            exclude_files.emplace(imprint_artifact);

            continue;  // ignore possible imprint file
        }

        files.push_back(ChildEntry{p, symstat}); // to process as final nodes
    }


    for(auto& cent : files){ // over final subnodes...
        if(exclude_files.find(cent.p) != exclude_files.end()){
            continue;
        }

        Node* cn = existingFileAt(cent.p, &cent.symstatus);
        if(cn == nullptr){
            // ToDo: report error???
            continue;
        }

        if(cn->valid_) {
            resolveFilesystemNode(*cn);
            n.children[cent.p.filename()] = cn; // append existing final node as child
            has_ref_descendants = has_ref_descendants || cn->has_refernces_;
        }
    }

    n.has_refernces_ = has_ref_descendants;
    n.resolved_ = NODE_RESOLVED;
}



}
#endif
