#ifndef thicket_context_resolve_ref_hpp
#define thicket_context_resolve_ref_hpp

#include "context.hpp" // not necessary, just help IDE to resolve symbols

namespace cornus_thicket {

inline void
detectRefnodeTargetType(Node& n){ // assuming final targets already collected
    Node* first_detected_final = NULL;

    auto& ftgs = n.final_targets;

    for(auto i = ftgs.begin(); i != ftgs.end(); i++){
        Node* tn = i->second;
        TargetType tt = tn->target_type;
        if(tt == UNKNOWN_TARGET_TYPE){
            report_error(
                    std::string("Node ") + n.path_.c_str()
                    + " target type undefined for target " + tn->path_.c_str()
                    , SEVERITY_ERROR
            );
        }else{
            if(first_detected_final == nullptr){
                first_detected_final = tn;
            }else{
                if(first_detected_final->target_type != tt){
                    report_error(
                            std::string("Node ") + n.path_.c_str()
                            + " ambiguous target types: 1-st target " + first_detected_final->path_.c_str()
                            + " 2-nd target :  " + tn->path_.c_str()
                            , SEVERITY_ERROR
                    );
                }
            }
        }
    }

    if(first_detected_final){
        n.target_type = first_detected_final->target_type;
    }

    if(n.target_type == UNKNOWN_TARGET_TYPE) {
        report_error(
                std::string("Reference Node ") + n.path_.c_str()
                + " target type undefined (node has " + std::to_string(n.final_targets.size()) + " final targets )"
                , SEVERITY_ERROR
        );
    }

    if(n.target_type == FILE_NODE && n.final_targets.size() > 1){
        report_error(
                std::string("Reference Node ") + n.path_.c_str()
                + " has regular file target but has more than one final target nodes"
                , SEVERITY_ERROR
        );

    }
}

inline void
collectFinalTargets(Node& n){
    auto& tgs = n.targets;
    auto& ftgs = n.final_targets;

    auto add_final_target = [&](const std::string& key,  Node* ftn){
        auto alredy_found = ftgs.find(key);
        if(alredy_found != ftgs.end()){
            if(alredy_found->second != ftn){
                report_error("Duplicated nodes found (internal error)", SEVERITY_PANIC); // ToDo: eleborate error !!!
            }
        }else{
            ftgs[key] = ftn;
        }
    };

    for(auto itg = tgs.begin(); itg != tgs.end(); itg++){ // over targets
        // collect final targets:
        Node* tn = *itg; // current immediate target
        if(tn->ref_type == FINAL_NODE){
            add_final_target(tn->path_, tn);
        }else if(tn->ref_type == REFERENCE_NODE){
            auto& curtg_ftgs = tn->final_targets;
            // loop over final targets of targets:
            for(auto iftg = curtg_ftgs.begin();  iftg != curtg_ftgs.end(); iftg++){
                const auto& key = iftg->first;
                Node* ft = iftg->second;
                add_final_target(key, ft);
            }
        }else{
            // report error? (undefined ref type)
        }
    }

    detectRefnodeTargetType(n);
}

inline void
Context::collectRefnodeChildren(Node& n){
    auto& tgs = n.targets;

    for(auto itg = tgs.begin(); itg != tgs.end(); itg++){ // over targets
        Node& tn = **itg;
        auto& tnch = tn.children;
        for(auto itg_ch = tnch.begin(); itg_ch != tnch.end(); itg_ch++){ // over target's children
            const string_t& tch_name = itg_ch->first;
            Node* tch_node = itg_ch->second;

            auto my_child_entry = n.children.find(tch_name);
            Node* my_child = nullptr;

            if (my_child_entry != n.children.end()){ // child already exists?
                my_child = my_child_entry->second;
            }else{
                my_child = create<Node>(n.path_/tch_name);
                n.children[tch_name] = my_child;
            }

            my_child->ref_type = REFERENCE_NODE;
            my_child->targets.push_back(tch_node);
        }
    }
}


void
Context:: resolveReference(Node& n){ // assuming targets are resolved
    collectFinalTargets(n);
    collectRefnodeChildren(n);
    for(auto& pair: n.children){
        resolve(*(pair.second));  // resolve children
    }
}

} // namespace

#endif
