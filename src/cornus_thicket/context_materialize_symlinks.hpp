#ifndef context_materialize_symlinks_hpp
#define context_materialize_symlinks_hpp

#include "context.hpp" // not necessary, just help IDE to resolve symbols

namespace cornus_thicket {

inline void mk_symlink(Node& n, const Node& to){

    // create symlink:
    fs::path base = n.path_.parent_path();
    fs::path link_target_canon = to.path_ ; // n.final_targets.begin()->second->path_;
    fs::path link_target = fs::relative(link_target_canon, base);

    std::error_code er;

    if(n.target_type == DIR_NODE) {
        create_directory_symlink(link_target, n.path_, er);
    }else if(n.target_type == FILE_NODE) {
        create_symlink(link_target, n.path_, er);
    }

    if(er){
        report_error(
                std::string("Failed to create symlink \n    from: ")
                + p2s(n.path_)
                + "\n    to: "
                + p2s(link_target_canon)
                , SEVERITY_ERROR
        );
    }
}


inline void
Context::materializeAsSymlinks(Node& n){
    if(n.ref_type == REFERENCE_NODE){  // (may return from the inside)
        if(n.targets.size() == 1 && path_in_scope(n.targets[0]->path_)){
            mk_symlink(n, *n.targets[0]); // the target shall be resolved, so just make symlink to it
            return; // do not recurse further, link is enough
        }else if(n.final_targets.size() == 1) {// if exactly one final target
            Node* ft =  n.final_targets.begin()->second;

            // is the target complete (fully final) or located inside materialization scope?
            if(!ft->has_ref_descendants_ || path_in_scope(ft->path_))
            {
                // then it can be symlinked
                mk_symlink(n, *ft);
                return; // do not recurse further, link is enough
            }

            // else (in case of "foreign" target containing mountpoints)
            // the target can not be symlinked and shall be materialized below
        }

        // if node can not be symlinked:

        if(n.target_type == DIR_NODE){
            // materialize node as directory:
            fs::create_directory(n.path_); // ToDo: catch ???
        }

    }

    // for final (filesystem) nodes or references with more than one target:
    for(auto& pair : n.children){
        materializeAsSymlinks(*pair.second);
    }
}

void Context::materializeAsSymlinks(){ // materializes all under the scope
  materializeAsSymlinks(*nodeAt(scope_));
}


}

#endif
