#ifndef cornus_thicket_filter_hpp
#define cornus_thicket_filter_hpp

#include <string>


namespace cornus_thicket {


enum DescendantsMatch{
    DESC_NONE,
    DESC_POSSIBLE,
    DESC_ALL
};

struct FilterMatch
{
    bool matches;

    DescendantsMatch desc_match;
};


class FilterBase // just a "concept"
{
public:
    std::string // returns error str
    setFilter(const std::string& filter_str);

    FilterMatch
    match(const char* s);
};



// temporary implementation (only recursive universal and shallow universal filters are supported)
class Filter
    : public FilterBase
{
public:
    std::string // returns error str
    setFilter(const std::string& filter_str);

    FilterMatch
    match(const char* s);

private:

    enum FilterType{FLTTYPE_NONE, U_RECURSIVE, U_SHALLOW};

    FilterType flttype = FLTTYPE_NONE;
};


// ........filter implementation (temporary, only universal (**/*) and shallow universal (*):

inline std::string
Filter::
setFilter(const std::string& filter_str){
    if(filter_str == "**/*"){
        flttype = U_RECURSIVE;
    }else if(filter_str == "*") {
        flttype = U_SHALLOW;
    }else{
        return std::string("Filter pattern ") + filter_str + " is not supported (only * and **/*)";
    }

    return std::string(); // Ok
};


inline FilterMatch
Filter::
match(const char* s){
    switch(flttype){
    case U_RECURSIVE:
    {
        FilterMatch ret = {true, DESC_ALL};
        return ret;
    }
    case U_SHALLOW:
    {
        if(s[0] == 0){ // empty string
            FilterMatch ret = {true, DESC_POSSIBLE};
            return ret;
        }else{
            FilterMatch ret = {true, DESC_NONE}; // stop recursion
            return ret;
        }
    }
    case FLTTYPE_NONE: ;// calm compiler
    } // switch

    //default: (hope impossible, invalid filter case):
    FilterMatch ret = {false, DESC_NONE};
    return ret;
}

} // namespace

#endif
