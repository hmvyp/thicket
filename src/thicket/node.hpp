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

    std::map<string_t, Node*> children;
};




} // namespace

#endif
