#ifndef cornus_thicket_config_hpp
#define cornus_thicket_config_hpp

#include "mutils.h"

#ifdef CORNUS_THICKET_CONFIG
     // allow custom config:
#   include CORNUS_THICKET_STRINGIZE(CORNUS_THICKET_CONFIG)
#endif

#ifndef CORNUS_THICKET_MOUNTPOINT_SUFFIX
#   define CORNUS_THICKET_MOUNTPOINT_SUFFIX ".thicket_mount.txt"
#endif

#endif
