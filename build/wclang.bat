echo $'\n'Building Thicket for native Linux platform using clang... $'\n'
set ODIR=output\windows
if not exist %ODIR% mkdir %ODIR%

clang -std=c++17 -O3 -o%ODIR%/thicket.exe ../src/cornus_thicket/main.cpp
