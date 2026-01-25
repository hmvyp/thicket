IDIR="output/include/cornus_thicket"
IFILE="$IDIR/rev_hash.h"
mkdir -p $IDIR
REVHASH="#define CORNUS_THICKET_REVISION_HASH \"$(git rev-parse --short HEAD)\""
echo $REVHASH > $IFILE
echo >> $IFILE

