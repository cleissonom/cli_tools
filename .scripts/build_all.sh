#!/bin/bash

for dir in "$PWD"/*; do
    if [ -d "$dir" ]; then
        echo "Building in $dir..."
        (cd "$dir" && make build)
    fi
done

echo -e "\033[1;32mAll builds done!\033[0m\n"
