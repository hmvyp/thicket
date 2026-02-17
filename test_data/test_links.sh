if [[ $1 == "w"  ]] ; then
    # (expecting WSL environment here)
    echo running Thicket as Windows executable...
    expath=/windows/thicket.exe
else
    echo running Thicket as linux executable...
    expath="$(uname -s)_$(uname -i)/thicket"
fi

../build/output/${expath} -root_lev=1 -var=varA:importedA -var=varB:importedB -var=varM:mounted_here -- root/scope

echo "Thicket returned code: $?"
echo ""


# make directory structure description:
. list_files.sh

echo comparing directory structure with the reference one...

if diff allfiles.txt allfiles_ref.txt ; then
    echo test passed
else
    echo test failed
fi
