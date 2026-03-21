echo clean all:
THICKET_PARAMS='-f -c -em=mounts -root_lev=1 root/scope'
REFERENCE_FILES='clean_ref.txt'
. run1test.sh

echo ""
echo "...cleaning succesful"

