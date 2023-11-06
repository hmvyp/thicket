#ifndef cornus_thicket_main_hpp
#define cornus_thicket_main_hpp

#include "context.hpp"
#include <string.h>
#include <array>

namespace cornus_thicket {


inline void print_tree(Node* nd){
    if(nd == nullptr){
        std::cout << "\n Error  NULL Node \n";
    }

    std::cout << "\n Node resolve status: " + std::to_string(nd->resolved_);

    std::cout << p2s(nd->path_) << " final targets:";
    for(auto& pair : nd->final_targets){
        std::cout << "\n        " << p2s(pair.second->path_);
    }

    for(auto& pair : nd->children){
        print_tree(pair.second);
    }
}


struct Option
{
    constexpr Option(const char* k, bool hasval = false): has_val(hasval), key(k){}
    constexpr Option(const char* k, const char* value):  Option(k, true) {is_set = true; val = value;}
    bool has_val;
    bool is_set = false;
    const char* key;
    const char* val = "";
};

inline Option opt_clean_only("-c");
inline Option opt_quiet("-q");
inline Option opt_force("-f"); // do not ask anything
inline Option opt_method("-m", "symlinks");
inline Option opt_print_tree("-p");
inline Option opt_dry_run("-d");
inline Option opt_end_of_options("--");


int run_thicket(fs::path root, fs::path scope){
    Context ctx(root, scope);

    if(opt_quiet.is_set){
        ctx.silent_ = true;
    }
    if(opt_force.is_set){
        ctx.force_ = true;
    }
    
    bool q = opt_quiet.is_set;

    if(!q){std::cout << "\nresolving tree nodes...";};
    ctx.resolve();

    if(opt_print_tree.is_set){
        print_tree(ctx.nodeAt(ctx.scope_));
    }


    if(!q){std::cout << "\ncleaning up previous artefacts...\n";};
    ctx.clean();

    if(!opt_clean_only.is_set){
        if(strcmp(opt_method.val, "symlinks") == 0){
            if(!q){std::cout << "\nstarting materialization process...\n";};
            
            ctx.materializeAsSymlinks(); // ToDo: return result ???
            
			if(!q){std::cout << "\n...something is done";};
        }else{
            std::cout << "\nError:unrecognized materialization method: " << opt_method.val << "\n";
            return 1;
        }
    }

    std::cout << "\n";

    return 0;
}

inline std::array<Option*, 7> all_options{{
    &opt_clean_only,
    &opt_quiet,
    &opt_force,
    &opt_method,
    &opt_print_tree,
    &opt_dry_run,
    &opt_end_of_options
}};

inline bool parse_option(char* s, bool& err){
    if(opt_end_of_options.is_set){
        return false;
    }


    for(Option* opt : all_options){
        if(strstr(s,opt->key) != s){
            continue;
        }

        size_t klen = strlen(opt->key);

        if(strlen(s) > klen){ // option value encountered
            if(!opt->has_val){
                err = true;
                std::cout << "\nError: extra characters in option " << opt->key << " Found: " << s;
                return false;
            }else if(s[klen != '=']){
                err = true;
                std::cout << "\nError: no = before value in option " << opt->key << " Found: " << s;
                return false;
            }else{
                opt->val =  s + klen +1;
            }
        }

        opt->is_set = true;
        return true;
    }

    if(opt_end_of_options.is_set && s[0] == '-'){
        err = true;
        std::cout << "\nError: no such option: "  << s;
        return false;
    }

    return false;
}




inline int
main(int nargs, char** args){

    size_t nparams = 0;

    /*
    if(nargs != 3){
        std::cout << "2 parameters required: root, scope \n (scope shall be relative to root)";
        return 1;
    }
    */

    char* root = args[1];
    char* scope = args[2];

    bool err = false;
    for(int i = 1; i < nargs; ++i){
        char* arg = args[i];

        bool is_option = parse_option(arg, err);
        if(err){
            return 1;
        }

        if(is_option){
            continue;
        }

        ++nparams;

        switch(nparams){
        case 1:
            root = arg;
            break;
        case 2:
            scope = arg;
            break;
        default:
            std::cout << "\nError: extra parameter found:  "  << arg;
            return 1;
        }
    }


    return run_thicket(root, scope);

}

}

#endif
