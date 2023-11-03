#ifndef cornus_thicket_main_hpp
#define cornus_thicket_main_hpp

#include "context.hpp"

namespace cornus_thicket {


inline void print_tree(Node* nd){
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
    Context ctx(root, scope);
    ctx.resolve();
    print_tree(ctx.nodeAt(ctx.scope_));
    ctx.clean();
    ctx.materializeAsSymlinks();
    return 0;
}



/*
struct Option
{
    bool is_set = false;
    std::array<char, 64> key;
    std::array<char, 64> val;
};

typedef std::array<Option*, N> = {
    &opt1,
    &opt2,
    ....
}
*/


int main(int nargs, char** args){

    if(nargs != 3){
        std::cout << "2 parameters required: root, scope \n (scope shall be relative to root)";
        return 1;
    }

    char* root = args[1];
    char* scope = args[2];

    return test1(root, scope);

}

}

#endif
