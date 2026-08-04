package main

import (
	"encoding/hex"
	"flag"
	"fmt"
	"os"
	"strings"
	"text/tabwriter"

	"candb/internal/db"
	"candb/internal/decode"
)

func cmdDecode(args []string) error {
	fs := flag.NewFlagSet("decode", flag.ExitOnError)
	dbFlag := addDBFlag(fs)
	parseFlags(fs, args)

	if fs.NArg() < 2 {
		return fmt.Errorf("usage: candb decode -db <file> <id> <hexdata>")
	}

	d, err := loadDB(*dbFlag)
	if err != nil {
		return err
	}

	id, err := db.ParseID(fs.Arg(0))
	if err != nil {
		return fmt.Errorf("invalid id %q: %w", fs.Arg(0), err)
	}
	hexStr := strings.ReplaceAll(strings.TrimPrefix(strings.TrimPrefix(fs.Arg(1), "0x"), "0X"), " ", "")
	data, err := hex.DecodeString(hexStr)
	if err != nil {
		return fmt.Errorf("invalid hex data %q: %w", fs.Arg(1), err)
	}

	msg, ok := d.Lookup(id)
	if !ok {
		return fmt.Errorf("no entry for %s (%d) in this database", formatIDHex(id), id)
	}
	if len(msg.Signals) == 0 {
		fmt.Printf("%s: no individual signals documented for this ID\n", formatIDHex(id))
		return nil
	}

	w := tabwriter.NewWriter(os.Stdout, 0, 2, 2, ' ', 0)
	fmt.Fprintln(w, "SIGNAL\tRAW\tDECODED\tCONFIDENCE")
	for _, sig := range msg.Signals {
		r := decode.Decode(sig, data)
		if r.Err != nil {
			fmt.Fprintf(w, "%s\t-\t(%v)\t%s\n", sig.Name, r.Err, orDash(sig.Confidence))
			continue
		}
		fmt.Fprintf(w, "%s\t%d (0x%X)\t%s\t%s\n", sig.Name, r.RawValue, r.RawValue, r.Decoded, orDash(sig.Confidence))
	}
	return w.Flush()
}
