echo $'\n'Building Thicket for native Linux platform using gcc c++14 + boost... $'\n'
if [[ -z "$1" ]] ; then
  echo "argument required: path to boost directory (containing include and lib folders)"
  echo "(lib folder must contain libboost_filesystem)"
  exit 1
fi

BOOSTDIR="$1";
ODIR="output/$(uname -s)_$(uname -i)"
mkdir -p "${ODIR}";

export LD_LIBRARY_PATH="$BOOSTDIR/lib"
# gcc -static -std=c++1y -O3 -DCORNUS_THICKET_BOOST_FS=1 -I../src -Ioutput/include -I$BOOSTDIR/include ../src/cornus_thicket/main.cpp "-o${ODIR}/thicket_b" -L$BOOSTDIR/lib -lstdc++ -lboost_filesystem -lboost_atomic

g++ -static -std=c++1y -O3 -DCORNUS_THICKET_BOOST_FS=1 -I../src -Ioutput/include -I$BOOSTDIR/include ../src/cornus_thicket/main.cpp "-o${ODIR}/thicket_b" -L$BOOSTDIR/lib  -lboost_filesystem -lboost_atomic

