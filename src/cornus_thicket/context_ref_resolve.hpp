#ifndef thicket_context_ref_resolve_hpp
#define thicket_context_ref_resolve_hpp

#include "context.hpp" // not necessary, just help IDE to resolve symbols

namespace cornus_thicket {

void
Context:: resolveReference(Node& n){ // assuming targets are resolved
    if(n.resolved_ > NODE_UNRESOLVED){
        return;
    }

    auto& tgs = n.targets;
    auto& ftgs = n.final_targets;

    auto add_final_target = [&](const std::string& key,  Node* ftn){
        auto alredy_found = ftgs.find(key);
        if(alredy_found != ftgs.end()){
            if(alredy_found->second != ftn){
                report_error("ambiguous final targets (internal error)", SEVERITY_PANIC); // ToDo: eleborate error !!!
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

        // collect children (as union of targets children) ( ToDo: ):

        // ToDo: ...
    }

}

}

#endif
