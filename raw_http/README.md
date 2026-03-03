# Raw HTTP Request Tool

## Overview

This C program uses libcurl to send raw HTTP requests configured from a `.txt` file. It supports `GET`, `POST`, `PUT`, `DELETE`, `PATCH`, `OPTIONS`, and `HEAD`, and saves the complete response (headers + body) to an output file.

The project now follows a modular layout similar to `salary_calculator`:

- `include/` for module contracts
- `src/` for implementations
- `tests/` for parser-focused regression checks

## Features

- **Raw HTTP Request Configuration**: Configure method, path, HTTP version, host, headers, and body via text file.
- **Complete Response Capture**: Saves full response headers and body.
- **Host Validation**: Adds `http://` when host does not include a protocol.

## Prerequisites

- GCC
- libcurl
- make

## Build

```bash
make build
```

## Run

```bash
./raw_http <input_file.txt> <output_file.txt>
```

## Run Tests

```bash
make test
```

## Example Request File

```txt
GET / HTTP/1.1
Host: example.com
User-Agent: raw_http/1.0
Accept: text/html
Connection: close
```

## Example

```bash
./raw_http example.txt response.txt
```

## Acknowledgements

[libcurl](https://curl.se/libcurl/) for providing a powerful library.
