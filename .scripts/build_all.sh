#!/bin/bash

FAILED=0
RAN_BUILDS=0

declare -a TARGET_DIRS=()

if [ "$#" -gt 0 ]; then
    for name in "$@"; do
        TARGET_DIRS+=("$PWD/$name")
    done
else
    for dir in "$PWD"/*; do
        if [ -d "$dir" ]; then
            TARGET_DIRS+=("$dir")
        fi
    done
fi

for dir in "${TARGET_DIRS[@]}"; do
    if [ ! -d "$dir" ]; then
        echo "Skipping $(basename "$dir"): directory not found"
        FAILED=1
        continue
    fi

    makefile_path="$dir/Makefile"

    if [ ! -f "$makefile_path" ]; then
        echo "Skipping $(basename "$dir"): no Makefile"
        continue
    fi

    if ! grep -Eq '^[[:space:]]*build:' "$makefile_path"; then
        echo "Skipping $(basename "$dir"): no build target"
        continue
    fi

    echo "Building in $dir..."
    RAN_BUILDS=1

    if ! (cd "$dir" && make build); then
        FAILED=1
        echo "Build failed in $dir"
    fi
done

if [ "$RAN_BUILDS" -eq 0 ]; then
    echo -e "\033[1;33mNo builds were run.\033[0m"
    exit 0
fi

if [ "$FAILED" -ne 0 ]; then
    echo -e "\033[1;31mSome builds failed.\033[0m"
    exit 1
fi

echo -e "\033[1;32mAll builds passed!\033[0m\n"
