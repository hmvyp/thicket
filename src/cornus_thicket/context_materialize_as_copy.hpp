#ifndef context_materialize_as_copy_hpp
#define context_materialize_as_copy_hpp

#include "context.hpp" // not necessary, just help IDE to resolve symbols

namespace cornus_thicket {


inline void
Context::materializeAsCopy(Node& n, bool symlinks_inside){
    if(n.ref_type == REFERENCE_NODE){
        if(n.final_targets.size() == 1) {// if exactly one final target
            Node* ft =  n.final_targets.begin()->second;

            // is the target located inside materialization scope?
            if(symlinks_inside && path_in_scope(ft->path_))
            {
                // then it can be symlinked

                // create symlink:
                fs::path base = n.path_.parent_path();
                fs::path link_target_canon = ft->path_ ; // n.final_targets.begin()->second->path_;
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

                return; // do not recurse further, link is enough
            }

            // else (in case of "foreign" target containing mountpoints)
            // the target can not be symlinked and shall be materialized here
        }

        // if node can not be symlinked:

        if(n.target_type == DIR_NODE){
            // materialize node as directory:
            fs::create_directory(n.path_); // ToDo: catch ???
        }else if (n.target_type == FILE_NODE){
            if(n.final_targets.size() == 1){
                Node* ft =  n.final_targets.begin()->second;
                std::error_code er;
                // copy file:
                fs::copy(
                       ft->path_,
                       n.path_,
                       fs::copy_options::copy_symlinks,
                       er
                );

                if(er){
                    report_error(
                            std::string("Failed to copy a file from: ")
                            + p2s(n.path_)
                            + "\n    to: "
                            + p2s(ft->path_)
                            , SEVERITY_ERROR
                    );
                }
            } // else: hope the error has been already reported (more than one target for regular file)
        }
    }


    // for final (filesystem) nodes or references which can not be symlinked:
    for(auto& pair : n.children){
        materializeAsCopy(*pair.second, symlinks_inside);
    }
}


void Context::materializeAsCopy(bool symlinks_inside){ // materializes all under scope
  materializeAsCopy(*nodeAt(scope_), symlinks_inside);
}


}

#endif
