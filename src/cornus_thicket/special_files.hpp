#ifndef cornus_thicket_fs_literals_hpp
#   define cornus_thicket_fs_literals_hpp

#include "utils.hpp"
#include "config.hpp"

namespace cornus_thicket {

THICKET_FS_LITERAL(mountpoint_suffix, CORNUS_THICKET_MOUNTPOINT_SUFFIX);

THICKET_FS_LITERAL(mirage_suffix, CORNUS_THICKET_MIRAGE_SUFFIX);

THICKET_FS_LITERAL(imprint_suffix, CORNUS_THICKET_IMPRINT_SUFFIX);


inline bool
is_thicket_mountpoint_description(
        const fs::path& p, // input parameter (path to mountpoint description file)
        fs::path* mountpoint_path // output parameter (path to mountpoint itself)
){
    return filepath_has_suffix(p, mountpoint_suffix(), mountpoint_path) ||
           filepath_has_suffix(p, mirage_suffix(), mountpoint_path);
}


inline bool
is_thicket_mountpoint(
        const fs::path& p,  // input parameter (path to possible mountpoint)
        fs::path* mountpoint_descr_path // output parameter (path to mountpoint description file)
){
    const fs::path::string_type* suffixes[] = {&mountpoint_suffix(), &mirage_suffix(), nullptr};

    for(const fs::path::string_type** psuff = suffixes; *psuff != nullptr; ++psuff){
        fs::path pm = p;  // will be a path to mountpoint description file

        pm.replace_filename(p.filename().native() + (**psuff));

        fs::file_status fstat = symlink_status(pm);

        if(fs::exists(fstat) && fs::is_regular_file(fstat)) {
            if(mountpoint_descr_path != nullptr){
                *mountpoint_descr_path = std::move(pm);
            }
            return true;
        }
    }

    return false; // it is not a mountpoint (not necessary an error)
}



inline bool
is_thicket_imprint(
        const fs::path& p, // input parameter (path to mountpoint description file)
        fs::path* mountpoint_path = nullptr // output parameter (path to mountpoint )
){
    return filepath_has_suffix(p, imprint_suffix(), mountpoint_path);
}



}
#endif
