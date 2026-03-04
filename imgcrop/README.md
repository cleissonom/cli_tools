# Imgcrop

## Overview

`imgcrop` is a Rust CLI to crop images to exact pixel dimensions with deterministic framing and output naming.

It supports:

- required target size (`--size WIDTHxHEIGHT`),
- optional `--input` path,
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
./imgcrop --size <WIDTH>x<HEIGHT> [--input <PATH>] [--keep-proportion <true|false>] [--frame-horizontal <left|center|right>] [--frame-vertical <top|center|bottom>] [--output <PATH>]
```

## Defaults

- `--keep-proportion=true`
- `--frame-horizontal=center`
- `--frame-vertical=center`

## Input fallback when `--input` is omitted

- If exactly one supported image exists in the current directory, `imgcrop` uses it.
- If zero or multiple supported images exist, the command fails with an explicit error asking for `--input`.

Supported extensions for fallback detection include:
`png`, `jpg`, `jpeg`, `webp`, `gif`, `bmp`, `tiff`, `tif`, `ico`, `tga`.

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
./imgcrop --size 800x600 --input ./photo.jpg

# Crop with framing controls while preserving proportion
./imgcrop --size 800x600 --input ./banner.jpg --frame-horizontal right --frame-vertical top

# Force non-proportional resize + explicit output
./imgcrop --size 300x300 --input ./wide.png --keep-proportion false --output ./out/square.png
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
