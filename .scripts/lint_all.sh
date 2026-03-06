#!/bin/bash

FAILED=0
RAN_LINTS=0
RAN_SHELL_LINT=0

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

run_rust_lint() {
    local dir="$1"

    if ! command -v cargo >/dev/null 2>&1; then
        echo "Rust lint failed in $dir: cargo not found"
        FAILED=1
        return
    fi

    echo "Linting Rust project in $dir..."
    RAN_LINTS=1

    if ! (cd "$dir" && cargo fmt --all --check); then
        FAILED=1
        echo "Rust format check failed in $dir"
    fi

    if ! (cd "$dir" && cargo clippy --all-targets --all-features -- -D warnings); then
        FAILED=1
        echo "Rust clippy check failed in $dir"
    fi
}

run_c_lint() {
    local dir="$1"
    local tmp_output

    tmp_output="$(mktemp)"

    echo "Linting C project in $dir..."
    RAN_LINTS=1

    if ! (cd "$dir" && make build) >"$tmp_output" 2>&1; then
        cat "$tmp_output"
        FAILED=1
        echo "C build lint failed in $dir"
        rm -f "$tmp_output"
        return
    fi

    cat "$tmp_output"

    if grep -Eiq 'warning:' "$tmp_output"; then
        FAILED=1
        echo "C compiler warnings found in $dir (treated as lint failure)"
    fi

    rm -f "$tmp_output"
}

run_shell_lint() {
    local sh_files=()
    local file

    if ! command -v shellcheck >/dev/null 2>&1; then
        echo "Skipping shell lint: shellcheck not found"
        return
    fi

    if command -v rg >/dev/null 2>&1; then
        while IFS= read -r file; do
            sh_files+=("$file")
        done < <(rg --files -g '*.sh')
    else
        while IFS= read -r file; do
            sh_files+=("$file")
        done < <(find . -type d \( -name .git -o -name target \) -prune -o -type f -name '*.sh' -print)
    fi

    if [ "${#sh_files[@]}" -eq 0 ]; then
        echo "Skipping shell lint: no .sh files found"
        return
    fi

    echo "Linting shell scripts..."
    RAN_SHELL_LINT=1

    if ! shellcheck "${sh_files[@]}"; then
        FAILED=1
        echo "Shell lint failed"
    fi
}

for dir in "${TARGET_DIRS[@]}"; do
    if [ ! -d "$dir" ]; then
        echo "Skipping $(basename "$dir"): directory not found"
        FAILED=1
        continue
    fi

    ran_for_dir=0

    if [ -f "$dir/Cargo.toml" ]; then
        run_rust_lint "$dir"
        ran_for_dir=1
    fi

    makefile_path="$dir/Makefile"
    if [ -f "$makefile_path" ] && grep -Eq '^[[:space:]]*build:' "$makefile_path"; then
        if find "$dir" -maxdepth 3 -type f -name '*.c' | grep -q .; then
            run_c_lint "$dir"
            ran_for_dir=1
        fi
    fi

    if [ "$ran_for_dir" -eq 0 ]; then
        echo "Skipping $(basename "$dir"): no lintable targets found"
    fi
done

if [ "$#" -eq 0 ]; then
    run_shell_lint
fi

if [ "$RAN_LINTS" -eq 0 ] && [ "$RAN_SHELL_LINT" -eq 0 ]; then
    echo -e "\033[1;33mNo lint checks were run.\033[0m"
    exit 0
fi

if [ "$FAILED" -ne 0 ]; then
    echo -e "\033[1;31mSome lint checks failed.\033[0m"
    exit 1
fi

echo -e "\033[1;32mAll lint checks passed!\033[0m"
