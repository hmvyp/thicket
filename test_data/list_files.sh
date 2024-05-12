find root | sort | while read -r line
do
    if [[ -L "$line" ]]
    then
        # symlink case
        echo "symlink: $(readlink $line)"
    elif [[ -d "$line" ]]
    then
        echo "dir: $line"
    else
        sum=$(md5sum "$line" | cut -d ' ' -f 1)
        echo "file: $line md5: $sum"
    fi 
  
done > allfiles.txt
