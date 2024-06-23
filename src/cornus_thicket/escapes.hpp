#ifndef cornus_thicket_escapes_hpp
#define cornus_thicket_escapes_hpp

#include <string>
#include <vector>
#include <map>
#include <optional>

namespace cornus_thicket {

//  Spaces and tabs shall be escaped  (\n \t) in file paths
//  Note: paths are never quoted, quotation mark (if encountered)
//  is treated just as a regular character


inline
std::string  substituteEscapes(
        const std::string& s
){
    using std::string;
    string res;
    for(size_t pos = 0 ;;){
        auto bspos = s.find('\\', pos);  // nearest backslash position
        res += s.substr(pos, bspos - pos); // bspos == npos case is Ok
        if(bspos == string::npos){
            break;
        }

        if(bspos + 1 == s.length()){
           break; // ignore final backslash (maybe report error?)
        }

        char c = s[bspos + 1];
        char cr = c; // replacement

        switch(c){
        case 's': cr = ' ';  break;
        case 't': cr = '\t'; break;
        case 'r': cr = '\r'; break;
        case 'n': cr = '\n'; break;
        case 'v': cr = '\v'; break;
        case 'f': cr = '\f'; break;
        case 'a': cr = '\a'; break;
        case '\\': cr = '\\'; break;
        }

        res += cr;

        if(bspos + 2 == s.length()){
           break;
        }

        pos = bspos + 2;
    }

    return res;

}

}


#endif
