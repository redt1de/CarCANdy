// candb is a CLI for looking up CAN IDs/signals in a CarCANdy CANDB JSON
// file and for decoding CarCANdy SD-card log files against it.
package main

import (
	"fmt"
	"os"
)

func main() {
	if len(os.Args) < 2 {
		printUsage()
		os.Exit(1)
	}

	var err error
	switch os.Args[1] {
	case "list":
		err = cmdList(os.Args[2:])
	case "show":
		err = cmdShow(os.Args[2:])
	case "decode":
		err = cmdDecode(os.Args[2:])
	case "parse":
		err = cmdParse(os.Args[2:])
	case "-h", "--help", "help":
		printUsage()
		return
	default:
		fmt.Fprintf(os.Stderr, "candb: unknown command %q\n\n", os.Args[1])
		printUsage()
		os.Exit(1)
	}

	if err != nil {
		fmt.Fprintf(os.Stderr, "candb: %v\n", err)
		os.Exit(1)
	}
}

func printUsage() {
	fmt.Fprint(os.Stderr, `candb -- lookup/decode tool for CarCANdy CAN ID databases

Usage:
  candb list   -db <file> [-bus <vehicleBus>]
  candb show   -db <file> <id>
  candb decode -db <file> <id> <hexdata>
  candb parse  -db <file> <logfile> [-id <id1,id2,...>] [-bus <n>]
               [-min-confidence <level>] [-format table|csv] [-unknown]

<file>    can also be given via the CANDB_FILE environment variable.
<id>      a CAN ID, either hex ("0x204") or decimal ("516").
<hexdata> frame payload bytes as hex, e.g. "C0007D014B000000".

Run "candb <command> -h" for a command's full flag list.
`)
}
