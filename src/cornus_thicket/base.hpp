#ifndef cornus_thicket_base_hpp
#define cornus_thicket_base_hpp

#include "config.hpp"

#include <string>
#include <cstring>
#include <sstream>
#include <list>
#include <vector>
#include <map>
#include <unordered_map>

#include <filesystem>
#include <memory>
#include <iostream>

namespace cornus_thicket {

namespace fs = ::std::filesystem;

using char_t = fs::path::value_type;
using string_t = fs::path::string_type;

using errcode_t = int;

enum Severity{
    SEVERITY_NONE = 0,
    SEVERITY_WARNING = 20,
    SEVERITY_ERROR = 40, // program can proceed, but the results may be incomplete
    SEVERITY_PANIC = 100 // program can not proceed
};


inline int error_count = 0;

inline int shown_errors_count = 0;

inline int max_err_order = 0;

void report_error(
        std::string err,
        Severity sev,
        int err_order = 1 // suppress further errors with lesser order
){
    if(err_order > max_err_order){
        max_err_order = err_order;
    }

    bool shown = false;

    if(err_order >= max_err_order){
        auto errs =  std::string("\nError: ") + err  + "\n";
        std::cerr << errs;
        shown = true;
    }

    if(sev >= SEVERITY_ERROR){
       ++error_count;
       if(shown){
           ++shown_errors_count;
       }
    }


    if(sev >= SEVERITY_PANIC){
       //throw errs;
       exit(111);
    }
};

struct ObjectFactory;

struct ObjectBase
{
    ObjectBase() = default;
    ObjectBase(ObjectBase&&) = delete;
    virtual ~ObjectBase() = default;

    ObjectFactory* getFactory(){return factory_;}

private:
    friend class ObjectFactory;
    ObjectFactory* factory_ = nullptr;
};


struct ObjectFactory
{
    ObjectFactory() = default;

    ObjectFactory(ObjectFactory&&) = delete; // do not move/copy


    // Temporary implementation (arena allocator seems more appropriate here):

    template<typename ObjectType, typename... CtorArgs>
    ObjectType* create(CtorArgs... cargs){
        auto p = new ObjectType(cargs...);
        p->factory_ = this;
        all_objects_.emplace_back(p);
        return p;
    }

    void free_all(){
        all_objects_ = std::list<std::unique_ptr<ObjectBase> > ();
    }

    ~ObjectFactory(){free_all();}

private:
    std::list<std::unique_ptr<ObjectBase> > all_objects_;
};


} //namespace cornus_thicket


#endif
