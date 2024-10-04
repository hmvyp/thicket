if [[ $1 == "w"  ]] ; then
    # (expecting WSL environment here)
    echo running Thicket as Windows executable...
    expath=/windows/thicket.exe
else
    echo running Thicket as linux executable on x86_64 platform...
    expath=Linux_x86_64/thicket
fi

../build/output/${expath} -f -c -em=mounts -root_lev=1 root/scope

