#ifndef context_materialize_symlinks_hpp
#define context_materialize_symlinks_hpp

#include "context.hpp" // not necessary, just help IDE to resolve symbols

namespace cornus_thicket {

inline void
Context::
mk_symlink(Node& n, const Node& to){

    // create symlink:
    fs::path base = n.path_.parent_path();
    fs::path link_target_canon = to.path_ ; // n.final_targets.begin()->second->path_;
    fs::path link_target = fs::relative(link_target_canon, base);

    fs_errcode er;

    if(n.node_type == DIR_NODE) {
        create_directory_symlink(link_target, n.path_, er);
    }else if(n.node_type == FILE_NODE) {
        create_symlink(link_target, n.path_, er);
    }

    if(er){
        std::string add_explanation = (fs::exists(n.path_))
                ? "\n    Possible cause: artifact already exists from previous Thicket invocation (cleaning is needed)"
                : "";

        report_error(
                std::string("Failed to create symlink \n    from: ")
                + p2s(n.path_)
                + "\n    to: "
                + p2s(link_target_canon)
                + add_explanation
                , SEVERITY_ERROR
        );
    }

    std::string errs = imprint_wrap_.getImprint()->addArtifact(n, nsLINK);
}


inline void
Context::
materializeAsSymlinks(Node& n){
    ImprintControl impc(imprint_wrap_);
    if(n.is_mountpoint){
        impc.newImprint(addSuffix(n.path_ , imprint_suffix()));
    }

    if(n.ref_type == REFERENCE_NODE){  // (may return from the inside)
        if(!n.has_own_content_){
            if(n.targets.size() == 1 && path_in_scope(n.targets[0]->path_)){
                mk_symlink(n, *n.targets[0]); // the target shall be resolved, so just make symlink to it
                return; // do not recurse further, link is enough
            }else if(n.final_targets.size() == 1) {// if exactly one final target
                Node* ft =  n.final_targets.begin()->second;

                // is the target complete (fully final) or located inside materialization scope?
                if(!ft->has_refernces_ || path_in_scope(ft->path_))
                {
                    // then it can be symlinked
                    mk_symlink(n, *ft);
                    return; // do not recurse further, link is enough
                }

                // else (in case of "foreign" target containing mountpoints)
                // the target can not be symlinked and shall be materialized below
            }else if(n.final_targets.size() > 1 && n.node_type == NodeType::FILE_NODE){
                report_error(
                        std::string("Ambiguous regular file reference for:\n")
                            + p2s(n.path_)
                            + "\n    There are more than one target. The first two targets are: \n1)"
                            + n.targets[0]->path_as_string_
                            + "\n2)"
                            + n.targets[1]->path_as_string_
                        , SEVERITY_ERROR
                );
                return;
            }
        }

        // if node can not be symlinked:

        if(n.node_type == DIR_NODE){
            // materialize node as directory:

            fs::file_status fstat = fs::symlink_status(n.path_);

            if(fs::exists(fstat)){
                report_error(
                        std::string("Can not create directory: the file already exists")
                        + p2s(n.path_)
                        + "\nThis probably is undeleted previous artefacts (or internal Thicket error)"
                        , SEVERITY_ERROR
                );


                // else (if the directory already exists) do nothing
            }else{
                try{
                    fs::create_directory(n.path_);
                    impc.getImprint()->addArtifact(n, nsCOPY);
                }catch(...){
                    report_error(
                            std::string("Error creating directory ")
                            + p2s(n.path_)
                            , SEVERITY_ERROR
                    );
                }
            }
        }else { // if(n.node_type == UNKNOWN_NODE_TYPE){
            report_error(
                    std::string("Node at  ")
                    + p2s(n.path_)
                    + "  shall be a directory, but actually is "
                    + verboseNodeType(n.node_type)
                    + "\n(This is likely caused by previous errors)"
                    , SEVERITY_ERROR
            );
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
