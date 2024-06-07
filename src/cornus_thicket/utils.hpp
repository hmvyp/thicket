#ifndef cornus_thicket_utils_hpp
#define cornus_thicket_utils_hpp

#include "mutils.h"
#include "base.hpp"

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


//.......................... helper template to implement string2path_string():

template<typename some_string_t>
some_string_t
string2some_string(const std::string& s);

template<>
std::wstring
string2some_string<std::wstring>(const std::string& s){
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> cvt;  // Deprecated?
    return cvt.from_bytes(s);
}

template<>
std::string
string2some_string<std::string>(const std::string& s){ return s;}


//..... convert an utf-8 string to a string in native filesystem::path encoding:

inline
string_t
string2path_string(const std::string& s){return string2some_string<string_t>(s); }



//...................... helper template to implement p2s() conversion function:

template<typename some_string_t>
std::string
some_string2string(const some_string_t& s);

template<>
std::string
some_string2string<std::wstring>(const std::wstring& s){
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> cvt;   // Deprecated?
    return cvt.to_bytes(s);
}

template<>
std::string
some_string2string<std::string>(const std::string& s){ return s;}


//....... convert a string in native filesystem::path encoding to utf-8 string:
inline
std::string
p2s(const string_t& s){ return some_string2string<string_t>(s); }


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
