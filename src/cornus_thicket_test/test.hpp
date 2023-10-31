#ifndef cornus_thicket_test_hpp
#define cornus_thicket_test_hpp

#include "../cornus_thicket/context.hpp"
#include <unistd.h>

namespace fs = ::std::filesystem;

namespace ct = ::cornus_thicket;

inline void print_tree(ct::Node* nd){
    if(nd == nullptr){
        std::cout << "\n Error  NULL Node \n";
    }

    std::cout << "\n Node resolve status: " + std::to_string(nd->resolved_);

    std::cout << nd->path_ << " final targets:";
    for(auto& pair : nd->final_targets){
        std::cout << "\n        " << pair.second->path_;
    }

    for(auto& pair : nd->children){
        print_tree(pair.second);
    }
}



int test1(fs::path root, fs::path scope){
    ct::Context ctx(root, scope);
    ctx.resolve();
    print_tree(ctx.nodeAt(ctx.scope_));
    ctx.clean();
    ctx.materializeAsSymlinks();
    return 0;
}

int cornus_thicket_test_main(char* src_path){

    std::cout << "\n wd " << fs::current_path().c_str() << "\n";
    auto p =  fs::path(src_path);
    std::cout << "\n file " << p.c_str() << "\n";

    auto pc = fs::canonical(p);
    std::cout << "\n canon " << pc.c_str() << "\n";
    usleep(10000);

    auto root = pc/".."/"test_data"/"root";
    auto scope = fs::path("scope");

    std::cout << "\n root " << root.c_str() << "\n scope:" << scope.c_str() << "\n";

    test1(root, scope);

    return 0;
}

#endif
