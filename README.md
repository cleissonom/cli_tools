# CLI Tools Collection

A multi-language set of focused command-line tools for everyday workflows.

## Tools

| Tool | Language | What it does | Build | Test |
| --- | --- | --- | --- | --- |
| `imgconvert` | Rust | Converts images to a target format with safe output-path validation. | `cd imgconvert && make build` | `cd imgconvert && make test` |
| `pypaste` | Rust | Compacts Python scripts for terminal pasting and copies output to clipboard. | `cd pypaste && make build` | `cd pypaste && make test` |
| `raw_http` | C (libcurl) | Sends raw HTTP requests from `.txt` files and prints/saves full responses. | `cd raw_http && make build` | `cd raw_http && make test` |
| `workpay` | C (libcurl) | Calculates earnings from worked time and converts currencies via AwesomeAPI. | `cd workpay && make build` | `N/A` (no `test` target in `Makefile`) |

Detailed per-tool docs:
- [imgconvert/README.md](imgconvert/README.md)
- [pypaste/README.md](pypaste/README.md)
- [raw_http/README.md](raw_http/README.md)
- [workpay/README.md](workpay/README.md)

## Prerequisites

### macOS
- Xcode Command Line Tools (`clang`, `make`, `install`)
- `curl`, `tar`
- Rust toolchain (`cargo`, `rustc`) for Rust tools (`imgconvert`, `pypaste`)

Quick setup with Homebrew:

```bash
brew install rust
```

### C dependencies
- `raw_http` and `workpay` are linked with `libcurl`.
- On macOS, `libcurl` is typically available through system toolchains.

## Build and Test

Use the repository scripts from the repository root:

```bash
./.scripts/build_all.sh
./.scripts/test_all.sh
```

Install built binaries from a local clone:

```bash
./.scripts/add_to_local.sh -y "$HOME/.local/bin/"
```

## Public macOS install (no clone)

Use the installer script directly from GitHub:

```bash
curl -fsSL https://raw.githubusercontent.com/cleissonom/cli_tools/main/install-macos.sh | bash
```

Install only selected tools into a user-local bin directory:

```bash
curl -fsSL https://raw.githubusercontent.com/cleissonom/cli_tools/main/install-macos.sh | \
  bash -s -- --tools imgconvert,pypaste --prefix "$HOME/.local/bin"
```

Installer behavior:
- Downloads a tarball from GitHub (no `git clone` required)
- Builds selected tools through `./.scripts/build_all.sh`
- Installs binaries to `--prefix` (default: `/usr/local/bin`)
- Uses `sudo` only when destination requires elevated permissions

Installer help:

```bash
bash install-macos.sh --help
```

## Quick Usage

`imgconvert`:

```bash
imgconvert ./photo.jpg webp ./out/photo.webp
```

`pypaste`:

```bash
pypaste --stdout ./script.py
```

`raw_http`:

```bash
raw_http raw_http/example.txt
```

`workpay`:

```bash
workpay --from USD --to BRL 25 08:30:45
```

## Repository Layout

```text
.
├── .scripts/      # helper scripts (test orchestration and local install utilities)
├── imgconvert/    # Rust image converter
├── pypaste/       # Rust python compactor + clipboard integration
├── raw_http/      # C raw HTTP request sender using libcurl
└── workpay/       # C earnings + currency conversion CLI
```

## License

Apache License 2.0. See [LICENSE](LICENSE).
