echo $'\n'Building Thicket for native Windows platform using clang from Windows command line tools... $'\n'
set ODIR=output\windows
if not exist %ODIR% mkdir %ODIR%

clang -std=c++17 -O3 -I../src -o%ODIR%/thicket.exe ../src/cornus_thicket/main.cpp
