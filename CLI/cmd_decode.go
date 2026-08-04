package main

import (
	"encoding/hex"
	"fmt"
	"os"
	"strings"
	"text/tabwriter"

	"candb/internal/db"
	"candb/internal/decode"

	"github.com/spf13/cobra"
)

var decodeCmd = &cobra.Command{
	Use:   "decode <id> <hexdata>",
	Short: "Decode one raw frame payload by hand",
	Args:  cobra.ExactArgs(2),
	RunE: func(cmd *cobra.Command, args []string) error {
		d, err := loadDB()
		if err != nil {
			return err
		}

		id, err := db.ParseID(args[0])
		if err != nil {
			return fmt.Errorf("invalid id %q: %w", args[0], err)
		}
		hexStr := strings.ReplaceAll(strings.TrimPrefix(strings.TrimPrefix(args[1], "0x"), "0X"), " ", "")
		data, err := hex.DecodeString(hexStr)
		if err != nil {
			return fmt.Errorf("invalid hex data %q: %w", args[1], err)
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
		fmt.Fprintln(w, "SIGNAL\tRAW\tDECODED")
		for _, sig := range msg.Signals {
			r := decode.Decode(sig, data)
			if r.Err != nil {
				fmt.Fprintf(w, "%s\t-\t(%v)\n", sig.Name, r.Err)
				continue
			}
			fmt.Fprintf(w, "%s\t%d (0x%X)\t%s\n", sig.Name, r.RawValue, r.RawValue, r.Decoded)
		}
		return w.Flush()
	},
}

func init() {
	rootCmd.AddCommand(decodeCmd)
}
