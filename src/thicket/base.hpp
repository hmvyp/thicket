#ifndef ftreeimporter_base_hpp
#define ftreeimporter_base_hpp

#include <string>
#include <list>
#include <vector>
#include <map>
#include <unordered_map>

#include <filesystem>
#include <memory>
#include <iostream>

namespace ftreeimporter {

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

void report_error(std::string err, Severity sev){
    auto errs =  std::string("Error: ") + err  + "\n";
    if(sev >= SEVERITY_PANIC){
       throw errs;
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


struct Node; // forward

} //namespace ftreeimporter


#endif
