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
inline Option opt_root_lev("-root_lev", true);
inline Option opt_end_of_options("--");


int run_thicket(Context& ctx){
    if(opt_quiet.is_set){
        ctx.silent_ = true;
    }
    if(opt_force.is_set){
        ctx.force_ = true;
    }
    
    bool q = opt_quiet.is_set;

    if(!q){std::cout << "\nresolving tree nodes...\n";};
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
        }else if(strcmp(opt_method.val, "copy") == 0 || strcmp(opt_method.val, "mixed") == 0){
            if(!q){std::cout << "\nstarting materialization process...\n";};

            ctx.materializeAsCopy(strcmp(opt_method.val, "mixed") == 0);

            if(!q){std::cout << "\n...something is done";};
       }else{
            std::cout << "\nError:unrecognized materialization method: " << opt_method.val << "\n";
            return 1;
        }
    }

    std::cout << "\n";

    return error_count != 0;
}

inline std::array<Option*, 8> all_options{{
    &opt_clean_only,
    &opt_quiet,
    &opt_force,
    &opt_method,
    &opt_print_tree,
    &opt_dry_run,
    &opt_root_lev,
    &opt_end_of_options
}};

inline void show_help(){
    std::cout << "\nThicket dependencies resolver. Command line: \n"
            "\n1st form:\n"
            "---------\n\n"
            "<thicket_executable> <options> [--] root scope\n\n"
            "Parameters:\n"
            "    root - the universe where dependency targets may be found\n"
            "    scope - a path relative to the root (!)\n"
            "            where dependencies shall be resolved (materialized)\n"
            "\n2nd form:\n"
            "---------\n\n"
            "<thicket_executable> -root_lev=N <other_options> [--] scope\n\n"
            "    N specifies the root as parent (N=1), grandparent(N=2), etc., of the scope directory\n"
            "\nParameters:\n"
            "    scope - a path (either absolute or relative to the current working(!) directory) \n"
            "            where dependencies shall be resolved (materialized)\n"
            "\nExample: thicket -root_lev=2 .\n"
            "\n  The example materializes dependencies in the current directory (dot passed as a parameter)"
            "\n  searching for them in the grandfather of the current directory\n"
            "\n\nAvailable options:\n"
            "------------------\n\n"
            "-c  clean only\n"
            "-f  force (do not ask before cleaning previous thicket artifacts)\n"
            "-q  quiet (implies -f), suppress console i/o except of error reporting\n"
            "-m=method  materialization method: symlinks (default), copy, mixed\n"
            "    symlinks - simlinks whereever possible\n"
            "    mixed - copy from the outside of the materialization scope, symlink inside\n"
            "    copy - always copy\n"
            ;
}

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
            }else if(s[klen] != '='){
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

    char* root = nullptr;
    char* scope = nullptr;
    int root_lev = -1; // root_lev option value (if any, negative otherwise)

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


        // root_lev option changes positional parameters meaning, so try to process the option:
        if(root_lev < 0 && opt_root_lev.val[0] != 0 ){
            try{
                auto lev = std::stoul(opt_root_lev.val, nullptr, 10);
                if(lev > 100){
                    std::cout << "\nError: root_lev option too large:  \n";
                    return 1;
                }

                root_lev = lev;

            }catch(...) {
                std::cout << "\nError: root_lev option is not a number  \n";
                return 1;
            }
        }

        ++nparams;

        switch(nparams){
        case 1:
            if(root_lev < 0){ // if root_lev not set
                root = arg;
            }else{
                scope = arg;
            }
            break;
        case 2:
            if(root_lev < 0){
                scope = arg;
                break;
            }
            //no break
        default:
            std::cout << "\nError: extra parameter found:  "  << arg;
            return 1;
        }
    }

    if(scope == nullptr){
        show_help();
        return 1;
    }

    if(root_lev < 0){
        Context ctx(root, scope);
        return run_thicket(ctx);
    }else{
        Context ctx(root_lev, scope);
        return run_thicket(ctx);
    }
}

}

#endif
