echo $'\n'Building Thicket for native Linux platform using gcc... $'\n'
ODIR="output/$(uname -s)_$(uname -i)"
mkdir -p "${ODIR}";

gcc -std=c++17 "-o${ODIR}/thicket" ../src/cornus_thicket/main.cpp  -lstdc++
