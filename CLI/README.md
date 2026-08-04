# candb

A small Go CLI for looking up CAN IDs/signals in a CarCANdy [CANDB](../DOC/CANDB/README.md)
JSON file, and for decoding a CarCANdy SD-card log file against it.

No external dependencies -- stdlib only (including `go/parser` to evaluate the
database's `formula` strings, e.g. `((b5<<8)+b6)/100`).

## Build

```sh
cd CLI
go build -o candb .
```

## Point it at a database

Every command takes `-db <file>`, or you can set `$CANDB_FILE` once instead:

```sh
export CANDB_FILE=../DOC/CANDB/2015_ford_mustang_gt.json
```

## Commands

### `candb list`

List every documented CAN ID.

```sh
candb list
candb list -bus HS3        # only IDs seen on vehicle bus "HS3"
```

### `candb show <id>`

Full detail for one message: byte offsets, formulas, enums, bitfields,
confidence, source, notes. `<id>` accepts hex (`0x204`) or decimal (`516`).

```sh
candb show 0x3B3
```

### `candb decode <id> <hexdata>`

Decode one raw frame payload by hand, e.g. to check a formula against a
sample byte string from a source doc.

```sh
candb decode 0x204 C0007D014B000000
```

### `candb parse <logfile>`

Parse a CarCANdy Teensy log (the `timestampMs,bus,id,extended,rtr,dlc,data`
CSV written to SD by `Teensy/src/Logging.cpp`) and decode every frame whose ID
is in the database.

```sh
candb parse mylog_00.csv
candb parse mylog_00.csv -id 0x204,0x156           # only these IDs
candb parse mylog_00.csv -bus 1                    # only CarCANdy bus 1
candb parse mylog_00.csv -min-confidence community-cross-confirmed
candb parse mylog_00.csv -format csv > decoded.csv # for a spreadsheet/plot
candb parse mylog_00.csv -unknown                  # also list undocumented IDs seen (to stderr)
```

Flags can go before or after the log file path.

## A note on confidence levels

`-min-confidence` filters signals, not whole frames, using this ranking
(low to high): `unconfirmed` / `conflict` < `community-single-source` <
`community-cross-confirmed` < `verified-on-vehicle`. See
[`DOC/CANDB/README.md`](../DOC/CANDB/README.md) for what each level means and
which signals in the 2015 Mustang GT database currently disagree between
sources -- decode those with real captures before trusting either
interpretation.

## A note on bus numbers

The log's `bus` column is CarCANdy's own bus number (which Teensy `FlexCAN_T4`
controller the frame arrived on), not the vehicle-side `HS1`/`HS3`/`MS` names
the database uses in `vehicleBus`. `candb parse -bus N` filters on the
former; `candb list -bus HS3` filters on the latter. Mapping one to the other
depends on your physical wiring -- see `DOC/pinout.md`.
