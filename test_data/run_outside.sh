# Run Thicket with absolute path to scope (scope is located outside the root)
# Note: all mountpoints in such "detached" scope must contain only records with target starting with "/"
# (that means relative to root, not to the mountpoint file)
#
# No test really performed, you can just see the resulting artifacts before cleaning up
 
if [[ $1 == "w"  ]] ; then
    # (expecting WSL environment here)
    echo running Thicket as Windows executable...
    expath=/windows/thicket.exe
else
    echo running Thicket as linux executable...
    expath="$(uname -s)_$(uname -i)/thicket"
fi

THICKET_LOC="../build/output/${expath}"

ABS_NEWSCOPE="$(realpath ./outside_root/new_scope)"
echo "Absolute path to ../outside_root/new_scope is: ${ABS_NEWSCOPE}"

${THICKET_LOC} -var=varA:importedA -var=varB:importedB -var=varM:mounted_here -- root ${ABS_NEWSCOPE}

echo "Thicket returned code: $?"
echo 

read -n 1 -r -s -p "Press any key to clean created artifacts..."

${THICKET_LOC} -c root ${ABS_NEWSCOPE}


echo ""

