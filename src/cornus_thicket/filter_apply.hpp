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

    FilterMatch fm = flt.match(cur_target_node.path_as_string_.c_str() + start_filtering_from);
    if(!fm.matches) {
        return nullptr;
    }

    if(fm.desc_match == DESC_ALL){
        cur()->has_own_content_  = false;
        cur()->targets.push_back(&cur_target_node);
        return cur();
    }else if(fm.desc_match == DESC_POSSIBLE){
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

        if(count == 0 && cur_target_node.node_type != FILE_NODE){
            if(cur_node){
                cur_node->has_own_content_  = true; // if cur_node passed as parameter
            }
            return nullptr; // do not create empty directories;
        }
    }else{ // DESC_NONE
        if(cur_target_node.node_type != FILE_NODE){ // if a directory
            if(cur_node){
                cur_node->has_own_content_  = true; // if cur_node passed as parameter
            }
            return nullptr; // do not create empty directories;
        }
    }

    cur()->has_own_content_  = cur_target_node.node_type != FILE_NODE; // filtered! (if a directory)
    cur()->targets.push_back(&cur_target_node);
    return cur();
}

} // namespace
#endif
