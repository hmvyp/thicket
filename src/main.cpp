#include <iostream>
#include "cornus_thicket/main.hpp"

#ifdef _WIN32 
#include <windows.h>
#endif


volatile std::wstring x = cornus_thicket::test_s2w("");
volatile std::string y = cornus_thicket::test_w2s(L"");

int main(int nargs, char** args){
#   ifdef _WIN32 
    unsigned cur_cp = GetConsoleOutputCP();
    SetConsoleOutputCP(65001);  // utf-8 to display localized file names
#   endif

    auto res = cornus_thicket::main(nargs, args);

#   ifdef _WIN32 
    if(cur_cp) {
        SetConsoleOutputCP(cur_cp);
    }else{
        // (report error) 
    }
#   endif

    return res;
}
