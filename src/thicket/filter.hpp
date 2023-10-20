#ifndef ftreeimporter_filter_hpp
#define ftreeimporter_filter_hpp

#include "base.hpp"

namespace ftreeimporter {


struct Filter
: public ObjectBase
{
    virtual bool is_empty() = 0;
    virtual bool is_wildcard() = 0;
    virtual Filter* descend(std::string child_name) = 0;
};

// Empty and universal (wildcard) filters (maybe make static instances?):

struct EmptyFilter // matches nothing
: public Filter
{
    virtual bool is_empty(){return true;};
    virtual bool is_wildcard() {return false;};
    virtual Filter* descend(std::string child_name){
        return getFactory()->create<EmptyFilter>();
    }
};

struct WildcardFilter // matches any
: public Filter
{
    virtual bool is_empty(){return false;};
    virtual bool is_wildcard() {return true;};
    virtual Filter* descend(std::string child_name){
        return getFactory()->create<WildcardFilter>();
    }
};


struct AndFilter
: public Filter
{
};

}
#endif
