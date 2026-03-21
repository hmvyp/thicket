echo '=================================== Thicket parameters:'
echo "${THICKET_PARAMS}"

if command -v wslinfo &> /dev/null; then
    # (expecting WSL environment here)
    echo running Thicket as Windows executable...
    expath=/windows/thicket.exe
else
    echo running Thicket as linux executable...
    expath="$(uname -s)_$(uname -i)/thicket"
fi


../build/output/${expath} ${THICKET_PARAMS}

RETCODE=$?
echo "Thicket returned code: $RETCODE"
if [[ $RETCODE != 0 ]]; then
    echo "test FAILED (Thicket return code ==  $RETCODE)"
    exit 1
fi
echo ""


# make directory structure description:
. list_files.sh

echo comparing directory structure with the reference one...
#diff allfiles.txt allfiles_ref.txt
diff allfiles.txt ${REFERENCE_FILES}
DIFFRETCODE=$?
if [[ $DIFFRETCODE != 0 || $RETCODE != 0 ]] ; then
    echo test FAILED
    exit 1
else
    echo '...comparison ok'
    echo ''
    echo ''
    echo ''
fi

