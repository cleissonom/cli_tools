#!/usr/bin/env bash

set -euo pipefail

REPO="cleissonom/cli_tools"
REF="main"
PREFIX="/usr/local/bin"
TOOLS_CSV="all"
YES=0

AVAILABLE_TOOLS=(imgconvert imgcrop pypaste raw_http workpay)

print_help() {
  cat <<'USAGE'
Install CLI tools from GitHub on macOS without cloning the repository.

Usage:
  install-macos.sh [options]

Options:
  --repo <owner/repo>      GitHub repository (default: cleissonom/cli_tools)
  --ref <git-ref>          Branch/tag/commit for tarball download (default: main)
  --tools <csv|all>        Comma-separated tools or 'all' (default: all)
  --prefix <dir>           Installation directory (default: /usr/local/bin)
  -y, --yes                Non-interactive mode
  -h, --help               Show this help message

Examples:
  install-macos.sh
  install-macos.sh --tools imgcrop,pypaste --prefix "$HOME/.local/bin"
  install-macos.sh --repo cleissonom/cli_tools --ref main -y
USAGE
}

log() {
  printf '[install-macos] %s\n' "$*"
}

fail() {
  printf '[install-macos] Error: %s\n' "$*" >&2
  exit 1
}

command_exists() {
  command -v "$1" >/dev/null 2>&1
}

contains_tool() {
  local needle="$1"
  local tool
  for tool in "${AVAILABLE_TOOLS[@]}"; do
    if [[ "$tool" == "$needle" ]]; then
      return 0
    fi
  done
  return 1
}

parse_tools_csv() {
  local csv="$1"

  if [[ "$csv" == "all" ]]; then
    printf '%s\n' "${AVAILABLE_TOOLS[@]}"
    return 0
  fi

  local seen=""
  local item

  IFS=',' read -r -a raw_items <<<"$csv"
  for item in "${raw_items[@]}"; do
    item="$(echo "$item" | tr -d '[:space:]')"

    [[ -z "$item" ]] && continue
    contains_tool "$item" || fail "unknown tool '$item'. valid values: ${AVAILABLE_TOOLS[*]}"

    case ",${seen}," in
      *",${item},"*)
        ;;
      *)
        seen+="${seen:+,}${item}"
        printf '%s\n' "$item"
        ;;
    esac
  done

  [[ -n "$seen" ]] || fail "no valid tools selected"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --repo)
      [[ $# -ge 2 ]] || fail "missing value for --repo"
      REPO="$2"
      shift 2
      ;;
    --ref)
      [[ $# -ge 2 ]] || fail "missing value for --ref"
      REF="$2"
      shift 2
      ;;
    --tools)
      [[ $# -ge 2 ]] || fail "missing value for --tools"
      TOOLS_CSV="$2"
      shift 2
      ;;
    --prefix)
      [[ $# -ge 2 ]] || fail "missing value for --prefix"
      PREFIX="$2"
      shift 2
      ;;
    -y|--yes)
      YES=1
      shift
      ;;
    -h|--help)
      print_help
      exit 0
      ;;
    *)
      fail "unknown option '$1'"
      ;;
  esac
done

[[ "$(uname -s)" == "Darwin" ]] || fail "this installer currently supports macOS only"

for cmd in curl tar make install; do
  command_exists "$cmd" || fail "required command not found: $cmd"
done

SELECTED_TOOLS=()
while IFS= read -r tool; do
  SELECTED_TOOLS+=("$tool")
done < <(parse_tools_csv "$TOOLS_CSV")

NEEDS_RUST=0
NEEDS_CC=0
for tool in "${SELECTED_TOOLS[@]}"; do
  case "$tool" in
    imgconvert|imgcrop|pypaste)
      NEEDS_RUST=1
      ;;
    raw_http|workpay)
      NEEDS_CC=1
      ;;
  esac
done

if [[ "$NEEDS_RUST" -eq 1 ]]; then
  command_exists rustc || fail "rustc not found. Install Rust (e.g. 'brew install rust')"
  command_exists cargo || fail "cargo not found. Install Rust (e.g. 'brew install rust')"
fi

if [[ "$NEEDS_CC" -eq 1 ]]; then
  command_exists cc || fail "C compiler not found. Install Xcode Command Line Tools"
fi

if [[ "$YES" -ne 1 ]]; then
  log "Repository: $REPO"
  log "Ref: $REF"
  log "Tools: ${SELECTED_TOOLS[*]}"
  log "Prefix: $PREFIX"
  printf 'Continue installation? [y/N] '
  read -r answer
  if [[ ! "$answer" =~ ^[Yy]$ ]]; then
    log "Cancelled"
    exit 0
  fi
fi

TMP_DIR="$(mktemp -d)"
cleanup() {
  rm -rf "$TMP_DIR"
}
trap cleanup EXIT

ARCHIVE_PATH="$TMP_DIR/source.tar.gz"
ARCHIVE_URL="https://codeload.github.com/${REPO}/tar.gz/${REF}"

log "Downloading source archive: $ARCHIVE_URL"
curl -fsSL "$ARCHIVE_URL" -o "$ARCHIVE_PATH"

log "Extracting archive"
tar -xzf "$ARCHIVE_PATH" -C "$TMP_DIR"

SOURCE_ROOT="$(find "$TMP_DIR" -mindepth 1 -maxdepth 1 -type d | head -n 1)"
[[ -n "$SOURCE_ROOT" ]] || fail "failed to locate extracted source directory"

log "Building tools via repository orchestrator script"
(
  cd "$SOURCE_ROOT"
  ./.scripts/build_all.sh "${SELECTED_TOOLS[@]}"
)

NEEDS_SUDO=0
if [[ ! -d "$PREFIX" ]]; then
  if ! mkdir -p "$PREFIX" 2>/dev/null; then
    NEEDS_SUDO=1
  fi
fi

if [[ ! -w "$PREFIX" ]]; then
  NEEDS_SUDO=1
fi

INSTALL_BIN=(install -m 0755)
if [[ "$NEEDS_SUDO" -eq 1 ]]; then
  command_exists sudo || fail "cannot write to '$PREFIX' and sudo is unavailable"
  INSTALL_BIN=(sudo install -m 0755)
fi

log "Installing binaries"
for tool in "${SELECTED_TOOLS[@]}"; do
  binary_path="$SOURCE_ROOT/$tool/$tool"
  [[ -f "$binary_path" ]] || fail "expected binary not found: $binary_path"
  "${INSTALL_BIN[@]}" "$binary_path" "$PREFIX/$tool"
  log "Installed $tool -> $PREFIX/$tool"
done

log "Installation complete"
if [[ ":$PATH:" != *":$PREFIX:"* ]]; then
  log "Add '$PREFIX' to your PATH if it is not already available in your shell"
fi
