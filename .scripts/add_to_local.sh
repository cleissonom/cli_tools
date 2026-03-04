#!/bin/bash

SKIP_CONFIRMATION=false

while getopts ":y" opt; do
  case $opt in
    y)
      SKIP_CONFIRMATION=true
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
  esac
done

shift $((OPTIND-1))

LOCAL_PATH=${1:-/usr/local/bin/}

if [ ! -w "$LOCAL_PATH" ]; then
    echo "Error: No write permission to $LOCAL_PATH"
    exit 1
fi

for dir in "$PWD"/*; do
    if [ -d "$dir" ]; then
        base_name=$(basename "$dir")
        executable_path="$dir/$base_name"
        
        if [ -f "$executable_path" ]; then
            echo "Installing $executable_path to $LOCAL_PATH..."

            if [ "$SKIP_CONFIRMATION" = true ]; then
                sudo cp "$executable_path" "$LOCAL_PATH"
                echo "Copied $executable_path to $LOCAL_PATH/$base_name"
            else
                read -p "Are you sure you want to copy $executable_path to $LOCAL_PATH? (y/n) " -n 1 -r
                echo    # move to a new line
                if [[ $REPLY =~ ^[Yy]$ ]]
                then
                    sudo cp "$executable_path" "$LOCAL_PATH"
                    echo "Copied $executable_path to $LOCAL_PATH/$base_name"
                else
                    echo "Skipping $executable_path..."
                fi
            fi
        else
            echo "Executable not found for $base_name, skipping..."
        fi
    fi
done

echo -e "\033[1;32mDone!\033[0m"
echo 
