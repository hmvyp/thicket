#ifndef thicket_context_clean_hpp
#define thicket_context_clean_hpp

#include "context.hpp" // not necessary, just help IDE to resolve symbols

namespace cornus_thicket {

inline void
Context::clean_using_mounts(const fs::path& p, std::map<fs::path, bool>& to_delete){
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
         }else if(is_thicket_imprint(p)){ // also delete imprints
             to_delete[p] = true;
         }
    }

    for (auto const& de : fs::directory_iterator{p}){
         auto& p = de.path();
         if(p.empty()){ // hope impossible
             continue;
         }

         if(to_delete.find(p) == to_delete.end() && !de.is_symlink() && de.is_directory()){
             clean_using_mounts(p, to_delete);
         }
    }
}


void Context::clean_using_mounts(){ // cleans all under the scope
    std::map<fs::path, bool> to_delete;
    clean_using_mounts(this->scope_, to_delete);
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

} // namespace

#endif
