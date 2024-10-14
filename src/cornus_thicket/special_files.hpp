#ifndef cornus_thicket_fs_literals_hpp
#   define cornus_thicket_fs_literals_hpp

#include "utils.hpp"
#include "config.hpp"

namespace cornus_thicket {

THICKET_FS_LITERAL(mountpoint_suffix, CORNUS_THICKET_MOUNTPOINT_SUFFIX);

THICKET_FS_LITERAL(imprint_suffix, CORNUS_THICKET_IMPRINT_SUFFIX);


bool
is_thicket_mountpoint_description(
        const fs::path& p, // input parameter (path to mountpoint description file)
        fs::path* mountpoint_path // output parameter (path to mountpoint itself)
){
    return filepath_has_suffix(p, mountpoint_suffix, mountpoint_path);
}


bool
is_thicket_imprint(
        const fs::path& p, // input parameter (path to mountpoint description file)
        fs::path* mountpoint_path = nullptr // output parameter (path to mountpoint )
){
    return filepath_has_suffix(p, imprint_suffix, mountpoint_path);
}



}
#endif
