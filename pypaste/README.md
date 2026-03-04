# PyPaste

## Overview

`pypaste` is a Rust CLI that prepares Python scripts for terminal pasting workflows.

Given a `.py` file path, it:

- removes trailing spaces/tabs from code lines,
- removes redundant blank lines while preserving code block indentation,
- keeps blank lines inside triple-quoted multiline strings,
- copies the compacted script to your clipboard.

## Build

```bash
make build
```

This creates the executable at:

- `pypaste/pypaste`

## Test

```bash
make test
```

## Usage

```bash
./pypaste <script.py>
```

Optional mode:

```bash
./pypaste --stdout <script.py>
```

`--stdout` prints the compacted content instead of copying it to clipboard.

## Example

Input:

```python
import os

def main():
    print("Hello, World!")

    if True:
      print("Hello, World!")
    else:
      print("Hello, World!")

    try:
      print("Hello, World!")
    except Exception as e:
      print(e)

    return 0
```

Output:

```python
import os

def main():
    print("Hello, World!")
    if True:
      print("Hello, World!")
    else:
      print("Hello, World!")
    try:
      print("Hello, World!")
    except Exception as e:
      print(e)
    return 0
```

## Clipboard Commands by Platform

- macOS: `pbcopy`
- Linux fallback order: `wl-copy`, `xclip -selection clipboard`, `xsel --clipboard --input`
- Windows: `clip`

If none are available, the tool exits with an actionable error.

## Repository Workflow

This project follows the same root workflow as other tools in this repository:

```bash
cd .scripts
./build_all.sh && ./add_to_local.sh -y
```

`./build_all.sh` runs `make build` in each project directory, and `./add_to_local.sh` installs each project executable from `<project>/<project>` into your local bin path.

## License

This project is licensed under the Apache License 2.0.

- Commercial and non-commercial use, modification, and redistribution are allowed under the license terms.
- Full terms: [../LICENSE](../LICENSE)
