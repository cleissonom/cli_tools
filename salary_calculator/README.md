# Salary Calculator

## Overview

`salary_calculator` computes payment from an hourly rate and worked time (`HH:MM:SS`), then converts the result between currencies using AwesomeAPI exchange rates.

## Features

- Help and usage output with `-h` / `--help`
- Strict input validation for hourly rate and time format
- Dynamic currency conversion with `--from` and `--to`
- Currency pair validation against AwesomeAPI supported pairs
- `Total earned` output formatted with currency symbol, thousands separators, and 2 decimal places

## Prerequisites

- GCC (GNU Compiler Collection)
- libcurl

## Build

```bash
make build
```

## Project Structure

The tool is split into focused modules under `src/` with public contracts in `include/`:

- `src/main.c`: Application orchestration and output rendering
- `src/sc_cli.c`: Help text and CLI argument parsing
- `src/sc_parse.c`: Input parsing and normalization helpers
- `src/sc_money.c`: Payment calculation and currency formatting
- `src/sc_quota.c`: Daily API quota tracking
- `src/sc_api.c`: HTTP requests, pair validation, and exchange rate retrieval
- `src/sc_config.c`: Shared visual constants (output colors)

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

## Daily API Limit Tracking

AwesomeAPI has a daily request limit of `100` requests. The tool tracks usage per day and enforces this limit before each API request.

Warnings shown when remaining quota is low:

```text
25 requests remaining
10 requests remaining
5 requests remaining
1 requests remaining
```

When the limit is reached:

```text
Requests limit reached
```

In this case, execution stops immediately.

Usage tracking storage:

- Default file: `~/.salary_calculator_api_usage`
- Optional override env var: `SALARY_CALCULATOR_USAGE_FILE`

## Examples

```bash
./salary_calculator 25 08:30:45
./salary_calculator --from EUR --to USD 40 07:15:00
./salary_calculator --from BTC --to BRL 0.005 01:00:00
./salary_calculator --from=BRL --to=EUR 35 06:45:30
```

Example output (USD -> BRL):

```text
Total earned in USD: $3,680.00
Total earned in BRL: R$ 19.483,39
```

Example output (BRL -> USD):

```text
Total earned in BRL: R$ 3.680,00
Total earned in USD: $691.01
```

Formatting notes:

- Total earned lines are always rounded to 2 decimal places.
- Symbol and separators follow the selected currency when known.
- For unknown currency codes, fallback format is `1,234.56 CODE`.

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

- Daily quota reached:

```text
Requests limit reached
```

## Notes

- The hourly rate is interpreted in the `--from` currency.
- Network access is required to validate pairs and fetch exchange rates.
- API request quota is tracked per local day (`YYYY-MM-DD`) and resets automatically on the next day.
- API provider: [AwesomeAPI](https://docs.awesomeapi.com.br/api-de-moedas)
