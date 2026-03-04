#!/bin/bash

FAILED=0
RAN_TESTS=0

for dir in "$PWD"/*; do
    if [ ! -d "$dir" ]; then
        continue
    fi

    makefile_path="$dir/Makefile"

    if [ ! -f "$makefile_path" ]; then
        echo "Skipping $(basename "$dir"): no Makefile"
        continue
    fi

    if ! grep -Eq '^[[:space:]]*test:' "$makefile_path"; then
        echo "Skipping $(basename "$dir"): no test target"
        continue
    fi

    echo "Running tests in $dir..."
    RAN_TESTS=1

    if ! (cd "$dir" && make test); then
        FAILED=1
        echo "Tests failed in $dir"
    fi
done

if [ "$RAN_TESTS" -eq 0 ]; then
    echo -e "\033[1;33mNo tests were run.\033[0m"
    exit 0
fi

if [ "$FAILED" -ne 0 ]; then
    echo -e "\033[1;31mSome tests failed.\033[0m"
    exit 1
fi

echo -e "\033[1;32mAll tests passed!\033[0m"
