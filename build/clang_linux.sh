echo $'\n'Building Thicket for native Linux platform using clang... $'\n'
ODIR="output/$(uname -s)_$(uname -i)"
mkdir -p "${ODIR}";

clang -std=c++17 -O3 -I../src "-o${ODIR}/thicket" -Ioutput/include ../src/cornus_thicket/main.cpp  -lstdc++
