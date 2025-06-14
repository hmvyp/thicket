#ifndef cornus_thicket_main_hpp
#define cornus_thicket_main_hpp

#include "context.hpp"
#include <string.h>
#include <array>

#define CORNUS_THICKET_APP_NAME "Thicket source tree resolver"
#define CORNUS_THICKET_VERSION "2.2.03"


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
    Option(const char* k, bool hasval = false, bool multival=false)
        : has_val(hasval)
        , is_multival(multival)
        , key(k)
    {}
    Option(const char* k, const char* value):  Option(k, true) {is_set = true; val = value;}
    bool has_val;
    bool is_multival;
    bool is_set = false;
    const char* key;
    const char* val = "";
    std::vector<const char*> multivalue;
};

inline Option opt_version("-version");
inline Option opt_clean_only("-c");
inline Option opt_quiet("-q");
inline Option opt_force("-f"); // do not ask anything
inline Option opt_method("-m", "symlinks");
inline Option opt_clean_method("-em", "imprint");  // erase method (imprint or mounts)
inline Option opt_print_tree("-p");
inline Option opt_dry_run("-d");
inline Option opt_root_lev("-root_lev", true);
inline Option opt_variable("-var", true, true); // multivalue
inline Option opt_end_of_options("--");


int run_thicket(Context& ctx){
    if(opt_quiet.is_set){
        ctx.silent_ = true;
    }
    if(opt_force.is_set){
        ctx.force_ = true;
    }
    
    bool q = opt_quiet.is_set;

    if(!opt_clean_only.is_set){ // then resolve...
    }

    if(strcmp(opt_clean_method.val, "imprint") == 0){
        if(!q){std::cout << "\ncleaning up previous artifacts using imprint from previous invocation...\n";};
        ctx.clean_using_imprint();
        if(error_count){
            std::cout << "\n... Artifacts cleaning FAILED";
            if(!opt_clean_only.is_set){
                std::cout << "\n    Further processing aborted due to cleaning failure\n";
            }
            return error_count != 0;
        }else{
            if(!q) { std::cout << "... cleaning complete.\n";}
        }
    } else  if(strcmp(opt_clean_method.val, "mounts") == 0) {
        if(!q){std::cout << "\ncleaning up previous artifacts using mountpoint description files...\n";};
        ctx.clean_using_mounts();
    }

    if(!opt_clean_only.is_set){

        if(!q){std::cout << "\nresolving tree nodes...\n";};

        ctx.resolve();

        if(opt_print_tree.is_set){
            print_tree(ctx.nodeAt(ctx.getScope()));
        }

        auto fin_report = [&]() -> void {
            if(error_count > 0) {
                std::cerr <<
                        "\n... the result may be incomplete due to errors.\nTotal errors: "
                        << error_count
                        << " Errors explained: "
                        << shown_errors_count
                        << "\n (some errors may be caused by others)";
            }else{
                if(!q){
                    std::cout << "\n... done";
                }
            }
        };

        if(strcmp(opt_method.val, "symlinks") == 0){
            if(!q){std::cout << "\nstarting materialization process...\n";};
            
            ctx.materializeAsSymlinks(); // ToDo: return result ???
            
			//if(!q){std::cout << "\n...something is done. Errors: " << error_count;};
			fin_report();
        }else if(strcmp(opt_method.val, "copy") == 0 || strcmp(opt_method.val, "mixed") == 0){
            if(!q){std::cout << "\nstarting materialization process...\n";};

            ctx.materializeAsCopy(strcmp(opt_method.val, "mixed") == 0);

            // if(!q){std::cout << "\n...something is done. Errors: " << error_count;};
            fin_report();
        }else{
            std::cout << "\nError:unrecognized materialization method: " << opt_method.val << "\n";
            return 1;
        }
    }

    if(!q || error_count !=0) {std::cout << "\n";}

    return error_count != 0;
}

inline std::array<Option*, 11> all_options{{
    &opt_version,
    &opt_clean_only,
    &opt_quiet,
    &opt_force,
    &opt_method,
    &opt_clean_method,
    &opt_print_tree,
    &opt_dry_run,
    &opt_root_lev,
    &opt_variable,
    &opt_end_of_options
}};

inline void show_help(){
    std::cout << "\n"
            CORNUS_THICKET_APP_NAME
            " version "
            CORNUS_THICKET_VERSION
            "\n\nCommand line: \n"
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
            "-version  prints Thicket version\n"
            "-c  clean only\n"
            "-f  force (clean previous artifacts without confirmation;\n"
            "    delete silently any extra object detected inside artifact directory)\n"
            "-q  quiet (in case of -em=mounts also implies -f), suppress console i/o except of error reporting\n"
            "-m=method  materialization method: symlinks (default), copy, mixed\n"
            "    symlinks - simlinks wherever possible\n"
            "    mixed - copy from the outside of the materialization scope, symlink inside\n"
            "    copy - always copy\n"
            "-em=erase_method artifacts cleaning method: imprint (default) or mounts\n"
            "    imprint - based on *.thicket_imprint files produced by previous invocation of Thicket\n"
            "        (imprint file lists previously created artifacts). This is the recommended method\n"
            "    mounts - an old cleaning method based on the current set of mountpoints under the scope.\n"
            "        This method is less precise and may be dangerous in some circumstances \n"
            "        (especially in OS virtualization environment if host's symlinks are not recognized by guest).\n"
            "        There is also a risk to delete something vital accidentally placed into artifact directory. \n"
            "        The 'mounts' method, however, is the last resort if .thicket_imprint files \n"
            "        are corrupted or accidentally deleted. \n"
            "-var  assigns a value to a variable\n"
            "    (a variable with name NAME can be used in mountpoint description files as ${NAME})\n"
            "    syntax:   var=NAME:VALUE\n"
            "    for example var=TARGET_OS:LINUX\n"
            "    There may be several -var options in the command line (a separate -var option for each variable)\n"
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
                const char* optval = s + klen +1;
                if(opt->is_multival){
                    opt->multivalue.push_back(optval);
                }else{
                    opt->val =  optval;
                }
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
        if(opt_version.is_set){
            std::cout <<
                    CORNUS_THICKET_APP_NAME
                    " version "
                    CORNUS_THICKET_VERSION
                    "\n";
                    return 0;
        }else{
            show_help();
            return 1;
        }
    }

    auto run_lambda = [&](Context& cx) -> int {
        auto err = addVarOptions(cx.getVarPool(), opt_variable.multivalue);
        if(err.empty()){
            auto before = std::chrono::system_clock::now();

            auto res = run_thicket(cx);

            auto after = std::chrono::system_clock::now();
            auto tdiff = after - before;

            if(! opt_quiet.is_set){
                std::cout << "  \n time spent: "
                        << std::chrono::duration_cast<std::chrono::milliseconds>(tdiff).count()

                        << " ms \n";
            }

            return res;
        }else{
            std::cout << "\nError: syntax in options " << err << "\n";
            return 1;
        }
    };

    if(root_lev < 0){
        Context ctx(root, scope);
        return run_lambda(ctx);
    }else{
        Context ctx(root_lev, scope);
        return run_lambda(ctx);
    }
}

}

#endif
