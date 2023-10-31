#ifndef cornus_thicket_node_hpp
#define cornus_thicket_node_hpp

#include "base.hpp"
#include "filter.hpp"

#include <set>
#include <unordered_set>


namespace cornus_thicket {

enum TargetType {
    UNKNOWN_TARGET_TYPE
    ,FILE_NODE
    ,DIR_NODE
    ,SYMLINK_NODE // ?? we do not follow symlinks but maybe we need to respect them as files?
};


enum ReferenceType {
    UNKNOWN_REFTYPE,
    FINAL_NODE,  // original file system object (file or directory) pre-existed before processing.
    REFERENCE_NODE, // refer to other nodes as content sources (may be materialized as directory or link)
};


enum ResolveStatus{
    NODE_UNRESOLVED,
    // ToDo: add NODE_RESOLVING (to detect cycles)
    NODE_RESOLVED,  // The whole subtree of all node descendants is built
    NODE_FAILED_TO_RESOLVE
};


struct Node
        : public ObjectBase
{
    TargetType target_type = UNKNOWN_TARGET_TYPE;
    ReferenceType ref_type = UNKNOWN_REFTYPE;

    Node(Node&&) = delete; // do not move/copy

    explicit Node(
            fs::path p // assuming canonical
    )
        : path_(p)
    {}

    const fs::path& get_path() {return path_;}

    bool valid_ = false;  // remains false if:
        // 1) reference type detection fails
        // 2) the node is final, but the corresponding filesystem object is unaccessible for any reason
        // 3) the node is reference, but it is impossible to collect immediate targets (referents)
        // Other error cases may lead to (valid == true) but (resolved_ == NODE_FAILED_TO_RESOLVE)

    ResolveStatus resolved_ = NODE_UNRESOLVED;

    fs::path path_ ; // canonical
    std::map<string_t, Node*> children;

    // the following fields are used for reference nodes only:

    std::vector<std::string>  mount_targets; // for mountpoint nodes only (string representation of targets)
    std::vector<Node*>  targets; // immediate targets (referents) of the reference node
    std::map<fs::path, Node*> final_targets; // final nodes only (union of targets of targets... of targets)
};


} // namespace

#endif
