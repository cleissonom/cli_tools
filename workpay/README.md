# Workpay

## Overview

`workpay` computes earnings from worked time and hourly rate, converts values with AwesomeAPI exchange rates, and can persist successful calculations to CSV for reporting.

## Features

- Calculation mode with strict input validation
- Currency conversion with `--from` and `--to`
- Optional persistence with `--save` and optional `--tag`
- Report mode with `summary`, `list`, and `export`
- CSV storage with fixed schema and env var override
- CSV/JSON exports for filtered report data

## Prerequisites

- GCC (GNU Compiler Collection)
- libcurl

## Build and Test

```bash
make build
make test
```

## CLI Usage

```bash
./workpay [--from CODE] [--to CODE] [--save] [--tag TAG] [--save-date DATE] [--exchange-rate RATE] [--extra-from AMOUNT] <hourly_rate> <hours:minutes:seconds>
./workpay report summary [--from-date YYYY-MM-DD] [--to-date YYYY-MM-DD] [--from CODE] [--to CODE]
./workpay report list [--from-date YYYY-MM-DD] [--to-date YYYY-MM-DD] [--from CODE] [--to CODE] [--limit N]
./workpay report export --format csv|json --output <PATH> [--from-date YYYY-MM-DD] [--to-date YYYY-MM-DD] [--from CODE] [--to CODE]
```

## Calculation Mode

### Arguments

- `<hourly_rate>`: non-negative decimal number
- `<hours:minutes:seconds>`: `HH:MM:SS` with `MM` and `SS` in `00..59`

### Flags

- `--from CODE`: source currency (default: `USD`)
- `--to CODE`: target currency (default: `BRL`)
- `--save`: save a successful calculation entry
- `--tag TAG`: optional tag for saved entries (requires `--save`)
- `--save-date DATE`: override saved timestamp (requires `--save`), accepted formats:
  - `YYYY-MM-DD` (saved as midnight local time)
  - `YYYY-MM-DDTHH:MM:SS`
- `--exchange-rate RATE`: required when using `--save-date`; uses provided rate instead of current API rate
- `--extra-from AMOUNT`: extra earnings in the source currency added before conversion

### Calculation Examples

```bash
./workpay 25 08:30:45
./workpay --from EUR --to USD 40 07:15:00
./workpay --from BTC --to BRL --save --tag invoice-42 0.005 01:00:00
./workpay --save --save-date 2026-03-01 --exchange-rate 5.10 --tag retro-entry 25 08:30:45
./workpay --from USD --to BRL --extra-from 120 23 154:22:12
```

## Report Mode

### `report summary`

Aggregates filtered entries by currency pair (`from_currency` + `to_currency`) to avoid mixing totals across different currencies.

```bash
./workpay report summary
./workpay report summary --from-date 2026-01-01 --to-date 2026-01-31
./workpay report summary --from USD --to BRL
```

### `report list`

Displays filtered entries in stable tabular form.

```bash
./workpay report list
./workpay report list --from-date 2026-01-01 --to-date 2026-01-31 --limit 20
./workpay report list --from USD --to BRL
```

### `report export`

Exports filtered entries to CSV or JSON. `--format` and `--output` are required.

```bash
./workpay report export --format csv --output /tmp/workpay.csv
./workpay report export --format json --output /tmp/workpay.json --from USD --to BRL
```

## Storage

Saved entries use append-only CSV storage.

- Default entries file: `~/.workpay_entries`
- Override env var: `WORKPAY_ENTRIES_FILE`
- API usage quota file (existing behavior): `~/.workpay_api_usage`
- API usage override env var: `WORKPAY_USAGE_FILE`

## CSV Schema

Header and column order are fixed:

```text
entry_id,created_at,from_currency,to_currency,hourly_rate,hours,minutes,seconds,total_from_currency,exchange_rate,total_to_currency,tag
```

Field notes:

- `entry_id`: Unix epoch timestamp (seconds)
- `created_at`: local datetime in `YYYY-MM-DDTHH:MM:SS`
- `tag`: optional, empty string when omitted

## Common Errors

- Invalid hourly rate:

```text
Invalid hourly rate. Use a non-negative number.
```

- Invalid time:

```text
Invalid time format. Use HH:MM:SS with MM/SS between 00 and 59.
```

- Invalid date flag value:

```text
Invalid value for --from-date. Use YYYY-MM-DD.
```

- Invalid save date:

```text
Invalid value for --save-date. Use YYYY-MM-DD or YYYY-MM-DDTHH:MM:SS.
```

- Invalid extra amount:

```text
Invalid value for --extra-from. Use a non-negative number.
```

- Missing required historical rate with save date:

```text
--exchange-rate is required when --save-date is provided.
```

- Invalid date range:

```text
Invalid date range: --from-date is after --to-date.
```

- Invalid limit:

```text
Invalid value for --limit. Use a positive integer.
```

- Missing export output path:

```text
Missing required option --output for report export.
```

- Empty filtered dataset:

```text
No entries found for the selected filters.
```

- Entries storage read/write failures:

```text
Failed to open entries file for writing: <path>
Failed to open entries file for reading: <path>
```

## Notes

- Existing calculation behavior is unchanged when `--save` is not used.
- Save happens only after successful calculation and exchange-rate resolution (API or `--exchange-rate`).
- Network access is required for calculation mode (AwesomeAPI).
- Report mode reads only local saved entries and does not call AwesomeAPI.

## License

This project is licensed under the Apache License 2.0.

- Commercial and non-commercial use, modification, and redistribution are allowed under the license terms.
- Full terms: [../LICENSE](../LICENSE)
