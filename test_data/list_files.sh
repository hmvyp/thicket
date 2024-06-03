# fix the sorting order:
export LC_ALL=C

find root | sort | while read -r line
do
    if [[ -L "$line" ]]
    then
        # symlink case
        echo "symlink: from: $line to: $(readlink $line)"
    elif [[ -d "$line" ]]
    then
        echo "dir: $line"
    else
        sum=$(md5sum "$line" | cut -d ' ' -f 1)
        echo "file: $line md5: $sum"
    fi 
  
done > allfiles.txt
