#ifndef cornus_thicket_filter_apply_hpp
#define cornus_thicket_filter_apply_hpp

#include "context.hpp" // not actually needed
#include "filter.hpp"

namespace cornus_thicket {

inline Node*  // nullptr on mismatch; otherwise cur_node (if provided) or newly created node
Context::
apply_filter(
        Node& cur_target_node,
        size_t start_filtering_from, // (in the target path string)
        const fs::path& parent_path, // used only if cur_node is null (to calculate new node path)
        Node* cur_node, // optional may be null (if needed, it is created inside)
        Filter& flt
){
    fs::path cur_path_memoized;

    auto cur_path = [&]() -> const fs::path& {
        if(cur_path_memoized.empty()){
            if(cur_node){
                cur_path_memoized = cur_node->path_;
            }else{
                cur_path_memoized = parent_path/cur_target_node.path_.filename();
            }
        }
        return cur_path_memoized;
    };

    auto cur = [&]()-> auto {
        if(cur_node == nullptr){
            cur_node = createNode(cur_path());
            cur_node->ref_type = REFERENCE_NODE;
        }
        return cur_node;

    };

    strview_type string_to_filter(cur_target_node.path_as_string_.c_str() + start_filtering_from);

    FilterMatch fm = flt.match(
            string_to_filter,
            cur_target_node.node_type == NodeType::DIR_NODE
    );


    if(fm.matches) {
        if(
                cur_target_node.node_type == NodeType::DIR_NODE
                && cur_target_node.children.empty()
                && !string_to_filter.empty() // prevent "unresolved error" for filtering root
        ){
            return nullptr; // do not create empty directories (except for filtering root)
        }

        cur()->targets.push_back(&cur_target_node); // (leave  cur()->has_own_content_ untouched)
        return cur();
    }

    // match failed. try to recurse:
    if(fm.no_recurse || cur_target_node.node_type == FILE_NODE){
        return nullptr;
    }

    // else (if directory)

    size_t count = 0;
    for(auto& trg_ch_entry : cur_target_node.children){
        Node* trg_ch = trg_ch_entry.second;
        Node* chn = apply_filter(
                *trg_ch, // hope not null
                start_filtering_from,
                cur_path(),
                nullptr,
                flt
        );

        if (chn == nullptr) {
            continue;
        }

        ++count;
        cur()->children[trg_ch_entry.first] = chn;
    }

    if(
            count == 0 // result directory node is empty
            && string_to_filter.size() != 0 // except for filtering root (avoid unresolved mountpoint  records)
    ){
        if(cur_node){
            cur_node->has_own_content_  = true; // if cur_node passed as parameter
        }
        return nullptr; // do not create empty directories;
    }


    cur()->has_own_content_  = true; // filtered!
    cur()->targets.push_back(&cur_target_node);
    return cur();
}

} // namespace
#endif
