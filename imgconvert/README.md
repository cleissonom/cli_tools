# ImgConvert

## Overview

`imgconvert` is a Rust CLI for converting images between file formats.

It accepts:

- an input image path,
- a target extension (`png`, `jpg`, `webp`, etc.),
- and an optional output image path.

When output path is omitted, the tool creates a new file beside the input using the same base name and the new extension.

## Build

```bash
make build
```

This creates the executable at:

- `imgconvert/imgconvert`

## Test

```bash
make test
```

## Usage

```bash
./imgconvert <input_image> <new_extension> [output_image]
```

## Examples

```bash
# Same name, new extension
./imgconvert ./photo.jpg png
# => ./photo.png

# Explicit output file path
./imgconvert ./photo.jpg webp ./out/hero-image.webp

# Output path without extension (extension is appended)
./imgconvert ./photo.png jpg ./out/hero-image
# => ./out/hero-image.jpg
```

## Notes

- Extensions are case-insensitive and may be passed with or without `.`.
- If the explicit output file extension conflicts with `<new_extension>`, the tool returns an error.
- To prevent accidental overwrite, the tool rejects conversions where input and output resolve to the same path.

## Repository Workflow

This project follows the same root workflow as other tools in this repository:

```bash
./build_all.sh && ./add_to_local.sh -y
```

`build_all.sh` runs `make build` in each project directory, and `add_to_local.sh` installs each project executable from `<project>/<project>` into your local bin path.

## License

This project is licensed under the Apache License 2.0.

- Commercial and non-commercial use, modification, and redistribution are allowed under the license terms.
- Full terms: [../LICENSE](../LICENSE)
