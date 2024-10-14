#ifndef cornus_thicket_utils_hpp
#define cornus_thicket_utils_hpp

#include "mutils.h"
#include "base.hpp"

#include <nowide/utf/convert.hpp>

#include <algorithm>
#include <codecvt>

namespace cornus_thicket {


// The macro THICKET_FS_LITERAL(constname, literal) declares a fs::path::string constant.
// The constant is named constname and initialized by a quoted string literal
// in a uniform way (regardless of character width used in fs::path::string)

#define THICKET_FS_LITERAL(constname, literal) \
template<typename CharT> \
inline constexpr const CharT* constname##_initfunc(){ /*returns lteral as  byte or wide character C-string */ \
    if constexpr (sizeof(CharT) == sizeof(char)) { \
        return CORNUS_THICKET_CONCAT( ,literal); \
    }else{ \
        return CORNUS_THICKET_CONCAT(L, literal);  /*wide character literal*/ \
    } \
};\
\
inline const string_t constname = constname##_initfunc<char_t>(); /* declare const variable of type fs::path::string_type */


//..... convert an utf-8 string to a string in native filesystem::path encoding:

inline
string_t
string2path_string(const std::string& s){
    return nowide::utf::convert_string<string_t::value_type, std::string::value_type>(s);
    //return string2some_string<string_t>(s);
}


//....... convert a string in native filesystem::path encoding to utf-8 string:
inline
std::string
p2s(const string_t& s){
    std::string tmp = nowide::utf::convert_string<std::string::value_type, string_t::value_type>(s);

    if(fs::path::preferred_separator != '/'){
                // = some_string2string<string_t>(s);
        std::replace(tmp.begin(), tmp.end(), '\\', '/');
        return tmp;
    }

    return tmp;
}

bool
filepath_has_suffix(
        const fs::path& p, // input parameter (path to mountpoint description file)
        const string_t& suff, // suffix description
        fs::path* path_wo_suffix = nullptr // output parameter: path without suffix (filled on success)
){
    auto fn = p.filename();
    auto s = fn.native();
    size_t len = s.length();
    if(len <= suff.length()){
        return false;
    }


    if(s.substr(len - suff.length(), suff.length()) == suff){
        if(path_wo_suffix != nullptr){
            *path_wo_suffix = p;
            path_wo_suffix->replace_filename(s.substr(0, len - suff.length()));
        }
        return true;
    }else{
        return false;
    }
}


fs::path addSuffix(const fs::path& p, const fs::path::string_type& suffix){
    fs::path tmp(p);
    tmp += suffix;
    return tmp;
}

//............................................................... trim string:


template<typename Predicate>
inline
std::string
trim(const std::string& s, Predicate pred)
{
   auto wsfront=std::find_if_not(s.begin(),s.end(),pred);
   auto wsback=std::find_if_not(s.rbegin(),s.rend(),pred).base();

   return (wsback<=wsfront ? std::string() : std::string(wsfront,wsback));
}

inline auto space_pred = [](unsigned char c){
    return std::isspace(c) &&  c != '#';
};

inline
std::string
trim(const std::string& s){
    return trim(s, space_pred);
}


}

#endif
