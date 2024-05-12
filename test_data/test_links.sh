rm allfiles.txt || true
. clear.sh
../build/output/Linux_x86_64/thicket -f -root_lev=1 root/scope
. list_files.sh
if diff allfiles.txt allfiles_ref.txt ; then
    echo test passed
else
    echo test failed
fi
