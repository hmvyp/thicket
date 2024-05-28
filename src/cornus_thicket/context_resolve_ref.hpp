#ifndef thicket_context_resolve_ref_hpp
#define thicket_context_resolve_ref_hpp

#include "context.hpp" // not necessary, just help IDE to resolve symbols

namespace cornus_thicket {

inline void
detectRefnodeType(Node& n){ // assuming targets already collected and resolved
    if(n.node_type != UNKNOWN_NODE_TYPE) {  // --T v2
        return; // if already assigned (mount path elements inside mountpoint may not have targets at all)
    }

    Node* first_detected_target = NULL;

    for(Node* tn : n.targets){ // over targets
        NodeType tt = tn->node_type;

        if(tt == UNKNOWN_NODE_TYPE){
            report_error(
                    std::string("Node ") + p2s(n.path_)
                    + " target type undefined for target " + p2s(tn->path_)
                    , SEVERITY_ERROR
            );
        }else{
            if(first_detected_target == nullptr){
                first_detected_target = tn;
            }else{
                if(first_detected_target->node_type != tt){
                    report_error(
                            std::string("Node ") + p2s(n.path_)
                            + " ambiguous target types: 1-st target " + p2s(first_detected_target->path_)
                            + " 2-nd target :  " + p2s(tn->path_)
                            , SEVERITY_ERROR
                    );
                }
            }
        }
    }

    if(first_detected_target){
        n.node_type = first_detected_target->node_type;
    }

    if(n.node_type == UNKNOWN_NODE_TYPE) {
        report_error(
                std::string("Reference Node ") + p2s(n.path_)
                + " target type undefined (node has " + std::to_string(n.final_targets.size()) + " final targets )"
                , SEVERITY_ERROR
        );
    }

    if(n.node_type == FILE_NODE && n.final_targets.size() > 1){
        report_error(
                std::string("Reference Node ") + p2s(n.path_)
                + " has regular file target but has more than one final target nodes"
                , SEVERITY_ERROR
        );
    }
}

inline void
collectFinalTargets(Node& n){
    auto& tgs = n.targets;
    auto& ftgs = n.final_targets;

    auto add_final_target = [&](const fs::path& key,  Node* ftn){
        auto alredy_found = ftgs.find(key);
        if(alredy_found != ftgs.end()){
            if(alredy_found->second != ftn){
                report_error("Duplicated nodes found (internal error)", SEVERITY_PANIC); // ToDo: eleborate error !!!
            }
        }else{
            ftgs[key] = ftn;
        }
    };

    for(auto& tn : tgs){ // over immediate targets
        if(tn->has_own_content_){ // --T v2  old code: tn->ref_type == FS_NODE){
            add_final_target(tn->path_, tn);
        }else if(tn->ref_type == REFERENCE_NODE){ // (and has no own content)
            // then collect final targets for every immediate target:
            auto& curtg_ftgs = tn->final_targets;

            for(auto& ftg_entry : curtg_ftgs){ // loop over final targets of targets
                const auto& key = ftg_entry.first;
                Node* ft = ftg_entry.second;
                add_final_target(key, ft);
            }
        }else{
            // report panic ?
        }
    }
}


inline void
Context::collectRefnodeChildren(Node& n){
    auto& tgs = n.targets;

    for(auto& tg : tgs){ // over targets
        Node& tn = *tg;
        auto& tnch = tn.children;
        for(auto itg_ch = tnch.begin(); itg_ch != tnch.end(); itg_ch++){ // over target's children
            const string_t& tch_name = itg_ch->first;
            Node* tch_node = itg_ch->second;

            /*
            auto my_child_entry = n.children.find(tch_name);
            Node* my_child = nullptr;

            if (my_child_entry != n.children.end()){ // child already exists?
                my_child = my_child_entry->second;
            }else{
                my_child = create<Node>(n.path_/tch_name);
                n.children[tch_name] = my_child;
            }
            */

            Node* my_child = this->ensureChild(&n, tch_name);

            my_child->ref_type = REFERENCE_NODE;
            my_child->valid_ = true;
            my_child->targets.push_back(tch_node);
        }
    }
}


void
Context:: resolveReferenceNode(Node& n, bool check_resolving){ // assuming targets are resolved
    if(n.resolved_ >= NODE_RESOLVED){
        return;
    }

    if(check_resolving && n.resolved_ == NODE_RESOLVING){
        report_error(std::string(
                "Circular dependency encountered while resolving reference node at ")
                    + p2s(n.path_),
                    SEVERITY_ERROR
        );
        return;
    }

    n.valid_ = true;
    n.resolved_ = NODE_RESOLVING;

    detectRefnodeType(n);

    collectFinalTargets(n);
    collectRefnodeChildren(n);

    for(auto& pair: n.children){
        resolve(*(pair.second));  // resolve children
    }

    n.resolved_ = NODE_RESOLVED;
}

} // namespace

#endif
