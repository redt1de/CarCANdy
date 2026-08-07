# candb

A small Go CLI for looking up CAN IDs/signals in a CarCANdy [CANDB](../DOC/CANDB/README.md)
JSON file, and for decoding a CarCANdy SD-card log file against it.

Built on [cobra](https://github.com/spf13/cobra) (via pflag), [go.bug.st/serial](https://github.com/bugst/go-serial)
for `candb live`, plus `go/parser` to evaluate the database's `formula`
strings, e.g. `((b5<<8)+b6)/100`.

## Build

```sh
cd CLI
go build -o candb .
```

## Point it at a database

Every command takes `--db <file>`, or you can set `$CANDB_FILE` once instead:

```sh
export CANDB_FILE=../DOC/CANDB/2015_ford_mustang_gt.json
```

## Commands

### `candb list`

List every documented CAN ID.

```sh
candb list
candb list --bus HS3        # only IDs seen on vehicle bus "HS3"
```

### `candb show <id>`

Full detail for one message: byte offsets, formulas, enums, bitfields, notes.
`<id>` accepts hex (`0x204`) or decimal (`516`).

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
candb parse mylog_00.csv --id 0x204,0x156           # only these IDs
candb parse mylog_00.csv --bus 1                    # only CarCANdy bus 1
candb parse mylog_00.csv --format csv > decoded.csv # for a spreadsheet/plot
candb parse mylog_00.csv --unknown                  # also list undocumented IDs seen (to stderr)
```

Flags can go before or after the log file path -- pflag (cobra's flag
parser) handles both. Long flags need the double dash (`--unknown`); a
single dash is reserved for shorthand letters, which this CLI doesn't define
any of.

### `candb live`

Same as `candb parse`, but reads live off the Teensy's Serial (USB) port
instead of a finished SD log file -- decodes frames as they arrive, and runs
until interrupted (Ctrl+C) or the port closes.

```sh
candb live --port /dev/ttyACM0                 # Linux; COM3 etc. on Windows
candb live --port /dev/ttyACM0 --id 0x204,0x156
candb live --port /dev/ttyACM0 --bus 1
candb live --port /dev/ttyACM0 --format csv >> live_log.csv
candb live --port /dev/ttyACM0 --unknown        # print each undocumented ID as it's seen (to stderr)
```

`Teensy/src/Logging.cpp` prints each CAN frame to Serial as `[CAN]
timestampMs,bus,id,extended,rtr,dlc,data` -- the same row format as the SD
CSV, just prefixed so it can be picked out of the rest of the Serial stream
(`[INFO]`/`[Logging]`/`[CANManager]` diagnostic lines share that same
stream). `candb live` reads every line: `[CAN] ...` lines get parsed and
decoded, and everything else is passed through to stderr so device
boot/status messages stay visible instead of being silently dropped.

`--baud` defaults to `115200` (matching the Teensy's `Serial.begin()` call),
though most USB-CDC serial ports (which is what the Teensy presents) ignore
the configured baud rate entirely.

Run `candb <command> --help` for a command's full flag list.

## A note on signal certainty

The database has no structured confidence/source field -- how sure a signal is,
and which community source(s) reported it, is written as prose in that
signal's `notes` instead (`candb show`/`decode`/`parse` all print it). See
[`DOC/CANDB/README.md`](../DOC/CANDB/README.md) for the sources behind the
2015 Mustang GT database and which signals currently disagree between
them -- decode those with real captures before trusting either
interpretation.

## A note on bus numbers

The log's `bus` column is CarCANdy's own bus number (which Teensy `FlexCAN_T4`
controller the frame arrived on), not the vehicle-side `HS1`/`HS3`/`MS` names
the database uses in `vehicleBus`. `candb parse --bus N` filters on the
former; `candb list --bus HS3` filters on the latter. Mapping one to the
other depends on your physical wiring -- see `DOC/pinout.md`.
