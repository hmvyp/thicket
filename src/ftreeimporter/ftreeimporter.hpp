#ifndef ftreeimporter_node_hpp
#define ftreeimporter_node_hpp

#include "base.hpp"
#include "filter.hpp"

#include <set>
#include <unordered_set>


namespace ftreeimporter {

enum TargetType {
    UNKNOWN_TARGET_TYPE,
    FILE_NODE,
    DIR_NODE,
};

struct MountEntry {
    Filter* filter;
    fs::path import_from;
};

enum ReferenceType {
    UNKNOWN_REFTYPE,
    FINAL_NODE,  // original file system object (file or directory) pre-existed before processing.
    REFERENCE_NODE, // refer to other nodes as content sources (may be materialized as directory or link)
};


struct Node
        : public ObjectBase
{
    TargetType target_type = UNKNOWN_TARGET_TYPE;

    ReferenceType ref_type = UNKNOWN_REFTYPE;

    Node(Node&&) = delete; // do not move/copy

    explicit Node(
            fs::path p // assumed canonical
    )
        : path_(p)
    {}

    const fs::path& get_path() {return path_;}

    bool valid_ = false;
    bool resolved = false;

    std::list<Node*>  targets;
    std::map<fs::path, Node*> final_targets;

    fs::path path_ ; // canonical

    std::map<std::string, Node*> children;
};


struct Context
        : public ObjectFactory
{
    constexpr const char* MNT_SUFFIX(){return ".fti_mount.txt";};

    fs::path root_;  // converted to canonical
    fs::path scope_; // converted to canonical

    std::map<fs::path, Node*> nodes;

    Context(
            fs::path root, // the whole universe
            fs::path scope // a folder where imports shall be resolved (path relative to root)
    ){

        std::error_code err;

        root_ = fs::canonical(root, err);
        if(!err){
            report_error("ftreeimporter::Context: Invalid root path", SEVERITY_PANIC);
        }

        scope_ =  fs::canonical(root_/scope, err);

        if(!err){
            report_error("ftreeimporter::Context: Invalid scope path", SEVERITY_PANIC);
        }
    }

    Node* resolve(fs::path p){
        Node* nd = nullptr;
        auto it = nodes.find(p);
        if(it == nodes.end()){
            nd = create<Node>(p);
            nodes[p] = nd;
        }else{
            nd = it->second;
        }

        if(nd->resolved){
            return nd;
        }

        fs::file_status fstat = status(p);

        if(nd->ref_type == UNKNOWN_REFTYPE) {
            auto mnt_description_path = p / MNT_SUFFIX();
            //auto stat =
            // ToDo: ...

        }

        return nullptr; // ToDo: ...

        //....

    }
};


} // namespace

#endif
