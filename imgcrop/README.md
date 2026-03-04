# Imgcrop

## Overview

`imgcrop` is a Rust CLI to crop images to exact pixel dimensions with deterministic framing and output naming.

It supports:

- required target size (`--size WIDTHxHEIGHT`),
- required positional input path (`<PATH>`),
- optional proportion preservation (`--keep-proportion true|false`),
- optional framing (`--frame-horizontal`, `--frame-vertical`),
- optional `--output` path.

## Build

```bash
make build
```

This creates the executable at:

- `imgcrop/imgcrop`

## Test

```bash
make test
```

## Usage

```bash
./imgcrop <PATH> --size <WIDTH>x<HEIGHT> [--keep-proportion <true|false>] [--frame-horizontal <left|center|right>] [--frame-vertical <top|center|bottom>] [--output <PATH>]
```

## Defaults

- `--keep-proportion=true`
- `--frame-horizontal=center`
- `--frame-vertical=center`

`<PATH>` must point to the input image file to crop.

## Output naming behavior

When `--output` is omitted, `imgcrop` writes to the input directory with this template:

- `{name}.{width}x{height}.{extension}`

Example:

- input: `photo.jpg`
- command: `--size 800x600`
- output: `photo.800x600.jpg`

If `--output` is provided without an extension, the input extension is appended.

To prevent accidental overwrite, input and output collisions are rejected.

## Examples

```bash
# Auto output naming => ./photo.800x600.jpg
./imgcrop ./photo.jpg --size 800x600

# Crop with framing controls while preserving proportion
./imgcrop ./banner.jpg --size 800x600 --frame-horizontal right --frame-vertical top

# Force non-proportional resize + explicit output
./imgcrop ./wide.png --size 300x300 --keep-proportion false --output ./out/square.png
```

## Repository Workflow

This project follows the same root workflow as other tools in this repository:

```bash
cd .scripts
./build_all.sh && ./add_to_local.sh -y
```

## License

This project is licensed under the Apache License 2.0.

- Commercial and non-commercial use, modification, and redistribution are allowed under the license terms.
- Full terms: [../LICENSE](../LICENSE)
