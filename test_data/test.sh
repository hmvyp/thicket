echo clean all:
THICKET_PARAMS='-f -c -root_lev=1 root/scope'
REFERENCE_FILES='clean_ref.txt'
. run1test.sh

echo test mountpoint as scope:
THICKET_PARAMS='-root_lev=2 -var=varA:importedA -var=varB:importedB -var=varM:mounted_here -- root/scope/src_all'
REFERENCE_FILES='mount_as_scope_ref.txt'
. run1test.sh

echo test cleaning after mountpoint as scope:
THICKET_PARAMS='-f -c -root_lev=2 -- root/scope/src_all'
REFERENCE_FILES='clean_ref.txt'
. run1test.sh

echo test root/scope as scope:
THICKET_PARAMS='-root_lev=1 -var=varA:importedA -var=varB:importedB -var=varM:mounted_here -- root/scope'
REFERENCE_FILES='allfiles_ref.txt'
. run1test.sh

echo ""
echo "...all tests passed"
