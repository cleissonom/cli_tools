# Raw HTTP Request Tool

## Overview

This C program uses libcurl to send raw HTTP requests configured from a `.txt` file. It supports `GET`, `POST`, `PUT`, `DELETE`, `PATCH`, `OPTIONS`, and `HEAD`.

Output target modes:

- With output file: writes the complete response (headers + body) to a file.
- Without output file: prints the complete response (headers + body) to console (`stdout`).

The project now follows a modular layout similar to `workpay`:

- `include/` for module contracts
- `src/` for implementations
- `tests/` for parser and CLI/output regression checks

## Features

- **Raw HTTP Request Configuration**: Configure method, path, HTTP version, host, headers, and body via text file.
- **Flexible Output Destination**: Save response to file or print directly to console.
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
./raw_http <input_file.txt> [output_file.txt]
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
# Write response to file
./raw_http example.txt response.txt

# Print response to console
./raw_http example.txt
```

## Input File Format

The request file is interpreted as:

1. Request line: `<METHOD> <PATH> <HTTP_VERSION>`
2. Required host line: `Host: <host-or-url>`
3. Optional headers (one per line)
4. Empty line
5. Optional body

Example:

```txt
POST /api/items HTTP/1.1
Host: api.example.com
Content-Type: application/json
X-Trace-Id: abc-123

{"name":"widget"}
```

## Module Layout

- `src/main.c`: orchestration and exit flow.
- `src/rh_cli.c`: CLI argument parsing and usage errors.
- `src/rh_parse.c`: request-file parsing and required-field validation.
- `src/rh_client.c`: host normalization, URL building, and curl execution.
- `src/rh_headers.c`: conversion of parsed headers to `curl_slist`.
- `src/rh_response.c`: response buffer lifecycle and curl callbacks.
- `src/rh_output.c`: output writing to stream/file.

Public contracts for these modules are in `include/`.

## Acknowledgements

[libcurl](https://curl.se/libcurl/) for providing a powerful library.

## License

This project is licensed under the Apache License 2.0.

- Commercial and non-commercial use, modification, and redistribution are allowed under the license terms.
- Full terms: [../LICENSE](../LICENSE)
