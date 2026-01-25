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

#if CORNUS_THICKET_BOOST_FS == 1
#   include <boost/filesystem.hpp>
#   include <boost/utility/string_view.hpp>
#   include <boost/optional.hpp>
#else
#   include <filesystem>
#   include <optional>
#endif

#include <memory>
#include <iostream>

namespace cornus_thicket {

#if CORNUS_THICKET_BOOST_FS == 1
    namespace fs = ::boost::filesystem;
    using fs_errcode = ::boost::system::error_code;
    using strview_type = boost::string_view;

    enum FileTypesEnum{
        not_found = boost::filesystem::file_type::file_not_found,
        regular = boost::filesystem::file_type::regular_file,
        directory = boost::filesystem::file_type::directory_file,
        // the following may not apply to some operating systems or file systems
        symlink = boost::filesystem::file_type::symlink_file,
        unknown = boost::filesystem::file_type::type_unknown  // file does exist, but isn't one of the above types or

    };

    template <typename T>
    using optional_alias = boost::optional<T>;

#else
    namespace fs = ::std::filesystem;
    using fs_errcode = ::std::error_code;
    using strview_type = std::string_view;

    using FileTypesEnum = std::filesystem::file_type;

    template <typename T>
    using optional_alias = std::optional<T>;
#endif

using char_t = fs::path::value_type;
using string_t = fs::path::string_type;

using errcode_t = int;

enum Severity{
    SEVERITY_NONE = 0,
    SEVERITY_WARNING = 20,
    SEVERITY_ERROR = 40, // program can proceed, but the results may be incomplete
    SEVERITY_PANIC = 100 // program can not proceed
};


struct ErrorCounts{
    int error_count = 0;

    int shown_errors_count = 0;

    int max_err_order = 0;

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
    }
};

ErrorCounts& getErrorCounts(){
    static ErrorCounts errc;
    return errc;
}

void report_error(
        std::string err,
        Severity sev,
        int err_order = 1 // suppress further errors with lesser order
){getErrorCounts().report_error(err, sev, err_order);}


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
