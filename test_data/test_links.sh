if [[ $1 == "w"  ]] ; then
    # (expecting WSL environment here)
    echo running Thicket as Windows executable...
    expath=/windows/thicket.exe
else
    echo running Thicket as linux executable on x86_64 platform...
    expath=Linux_x86_64/thicket
fi

# clear previous artefacts explicitly:
rm allfiles.txt || true
rm -rf root/scope/src_all || true
rm -rf root/scope/looop || true
rm -rf root/scope/src_another || true
rm -rf root/scope/src_import1file/file_from_a || true

../build/output/${expath} -f -c -root_lev=1 -var=varA:importedA -var=varB:importedB -var=varM:mounted_here  root/scope

# materialize thicket mounts:
../build/output/${expath} -f -root_lev=1 -var=varA:importedA -var=varB:importedB -var=varM:mounted_here -- root/scope

# make directory structure description:
. list_files.sh

# compare directory structure with the reference one:
if diff allfiles.txt allfiles_ref.txt ; then
    echo test passed
else
    echo test failed
fi
