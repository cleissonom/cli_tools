# Salary Calculator

## Overview

`salary_calculator` computes payment from an hourly rate and worked time (`HH:MM:SS`), then converts the result between currencies using AwesomeAPI exchange rates.

## Features

- Help and usage output with `-h` / `--help`
- Strict input validation for hourly rate and time format
- Dynamic currency conversion with `--from` and `--to`
- Currency pair validation against AwesomeAPI supported pairs

## Prerequisites

- GCC (GNU Compiler Collection)
- libcurl

## Build

```bash
make build
```

## Usage

```bash
./salary_calculator [--from CODE] [--to CODE] <hourly_rate> <hours:minutes:seconds>
```

### Arguments

- `<hourly_rate>`: Non-negative decimal number
- `<hours:minutes:seconds>`: Time worked in `HH:MM:SS` where `MM` and `SS` are between `00` and `59`

### Options

- `-h`, `--help`: Show help message
- `--from CODE`: Source currency code (default: `USD`)
- `--to CODE`: Target currency code (default: `BRL`)

Both `--from EUR` and `--from=EUR` styles are supported.

## Currency Pair Validation

The tool validates `FROM-TO` against AwesomeAPI available pairs before fetching rates.

- Supported pairs source: `https://economia.awesomeapi.com.br/json/available`
- Rate endpoint used by the tool: `https://economia.awesomeapi.com.br/json/last/FROM-TO`

If the pair is not supported, the tool returns an error such as:

```text
Unsupported currency pair: XXX-YYY
```

## Examples

```bash
./salary_calculator 25 08:30:45
./salary_calculator --from EUR --to USD 40 07:15:00
./salary_calculator --from BTC --to BRL 0.005 01:00:00
./salary_calculator --from=BRL --to=EUR 35 06:45:30
```

## Common Errors

- Invalid hourly rate:

```text
Invalid hourly rate. Use a non-negative number.
```

- Invalid time format:

```text
Invalid time format. Use HH:MM:SS with MM/SS between 00 and 59.
```

- Invalid currency code format:

```text
Invalid value for --from. Use only letters and digits (example: USD, BRL, BRLT, BTC).
```

- Unsupported pair:

```text
Unsupported currency pair: FROM-TO
```

## Notes

- The hourly rate is interpreted in the `--from` currency.
- Network access is required to validate pairs and fetch exchange rates.
- API provider: [AwesomeAPI](https://docs.awesomeapi.com.br/api-de-moedas)
