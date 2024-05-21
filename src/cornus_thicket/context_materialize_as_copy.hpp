#ifndef context_materialize_as_copy_hpp
#define context_materialize_as_copy_hpp

#include "context.hpp" // not necessary, just help IDE to resolve symbols
#include "context_materialize_symlinks.hpp" // mk_symlink()


namespace cornus_thicket {


inline void
Context::materializeAsCopy(Node& n, bool symlinks_inside){
    if(n.ref_type == REFERENCE_NODE){
        if(symlinks_inside) {  // (may return from the inside)
            if(n.targets.size() == 1 && path_in_scope(n.targets[0]->path_)){ // if a single immediate target is in scope
                mk_symlink(n, *n.targets[0]); // the target shall be resolved, so just make symlink to it
                return; // do not recurse further, link is enough
            }else if(n.final_targets.size() == 1) {// if exactly one final target
                Node* ft =  n.final_targets.begin()->second;
                if(path_in_scope(ft->path_))
                {
                    mk_symlink(n, *ft);
                    return; // do not recurse further, link is enough
                }
            }

            // We ignore exotic cases if some intermediate target is in the scope
            // while immediate and final targets are not.
        }

        // if node can not be symlinked:

        if(n.node_type == DIR_NODE){
            // materialize node as directory:
            fs::create_directory(n.path_); // ToDo: catch ???
        }else if (n.node_type == FILE_NODE){
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
