#include <iostream>
#include "cornus_thicket/main.hpp"


volatile std::wstring x = cornus_thicket::test_s2w("");
volatile std::string y = cornus_thicket::test_w2s(L"");

int main(int nargs, char** args){
    return cornus_thicket::main(nargs, args);
}
