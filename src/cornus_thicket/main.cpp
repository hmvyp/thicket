
#include "config.hpp"


#ifndef CORNUS_THICKET_NO_MAIN_CPP // (define the macro to use thicket as headers-only library)

#include "main.hpp"

#ifdef _WIN32 
#include <windows.h>
#endif


 int main(int nargs, char** args){
#   ifdef _WIN32 
    unsigned cur_cp = GetConsoleOutputCP();
    SetConsoleOutputCP(65001);  // utf-8 to display localized file names
#   endif

    auto before = std::chrono::system_clock::now();

    auto res = cornus_thicket::main(nargs, args);

    auto after = std::chrono::system_clock::now();
    auto tdiff = after - before;

    std::cout << "  \n time spent: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(tdiff).count()

            << " ms \n";

#   ifdef _WIN32 
    if(cur_cp) {
        SetConsoleOutputCP(cur_cp);
    }else{
        // (report error) 
    }
#   endif

    return res;
}

#endif
