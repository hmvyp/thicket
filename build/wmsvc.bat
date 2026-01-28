echo $'\n'Building Thicket for native Windows platform using msvc from Windows command line tools... $'\n'
set ODIR=output\windows
if not exist %ODIR% mkdir %ODIR%

cl /std:c++17 /O2 -I../src /Ioutput/include /Fe%ODIR%/thicket.exe ../src/cornus_thicket/main.cpp
