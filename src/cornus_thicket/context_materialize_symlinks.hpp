#ifndef context_materialize_symlinks_hpp
#define context_materialize_symlinks_hpp

#include "context.hpp" // not necessary, just help IDE to resolve symbols

namespace cornus_thicket {

inline void
Context::clean(const fs::path& p, std::map<fs::path, bool>& to_delete){
    if(fs::symlink_status(p).type() != fs::file_type::directory){
        return;
    }

    for (auto const& de : fs::directory_iterator{p}){
         auto& p = de.path();
         if(p.empty()){ // hope impossible
             continue;
         }

         fs::path mountpoint_path;
         if(
                 is_thicket_mountpoint_description(p, &mountpoint_path)
                 && !mountpoint_path.empty() // hope impossible
                 && fs::symlink_status(mountpoint_path).type() != fs::file_type::not_found
         ){
             to_delete[mountpoint_path] = true;
         }
    }

    for (auto const& de : fs::directory_iterator{p}){
         auto& p = de.path();
         if(p.empty()){ // hope impossible
             continue;
         }

         if(to_delete.find(p) == to_delete.end() && !de.is_symlink() && de.is_directory()){
             clean(p, to_delete);
         }
    }
}


inline void
Context::materializeAsSymlinks(Node& n){
    if(n.ref_type == REFERENCE_NODE){
        if(n.final_targets.size() == 1) {// if exactly one final target
            Node* ft =  n.final_targets.begin()->second;

            // is the target complete (fully final) or located inside materialization scope?
            if(!ft->has_ref_descendants_ || path_in_scope(ft->path_))
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
        }

    }


    // for final (filesystem) nodes or references with more than one target:
    for(auto& pair : n.children){
        materializeAsSymlinks(*pair.second);
    }
}

void Context::clean(){ // cleans all under the scope
    std::map<fs::path, bool> to_delete;
    clean(this->scope_, to_delete);
    if(to_delete.size() > 0){
        if(!silent_){
            std::cout << " \n Deleting generated files and directories: \n";

            for(auto& pair : to_delete){
                std::cout << p2s(pair.first) << "\n";
            }
        }

        char yn = 0;

        if(!silent_  && !force_){ // then ask before deleting
            do
            {
                std::cout << "\n delete? [y/n]\n";
                std::cin >> yn;
            }
            while( !std::cin.fail() && yn!='y' && yn!='n' );
        }

        std::error_code er;
        if(silent_ || force_ || yn == 'y'){
            for(auto& pair : to_delete){ // then delete
                auto& p = pair.first;
                remove_all(p, er );
                if(er){
                    std::cout << " \n Error while deleting " << p2s(p);
                }
            }
        }
    }
}


void Context::materializeAsSymlinks(){ // materializes all under scope
  materializeAsSymlinks(*nodeAt(scope_));
}


}

#endif
