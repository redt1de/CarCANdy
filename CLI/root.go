package main

import "github.com/spf13/cobra"

// dbPath backs the --db persistent flag shared by every subcommand.
// loadDB() (in common.go) resolves it, falling back to $CANDB_FILE.
var dbPath string

var rootCmd = &cobra.Command{
	Use:   "candb",
	Short: "Lookup/decode tool for CarCANdy CAN ID databases",
	Long: `candb looks up CAN IDs/signals in a CarCANdy CANDB JSON file and
decodes CarCANdy SD-card log files against it.

<id>      a CAN ID, either hex ("0x204") or decimal ("516").
<hexdata> frame payload bytes as hex, e.g. "C0007D014B000000".`,
	SilenceUsage:  true,
	SilenceErrors: true,
}

func init() {
	rootCmd.PersistentFlags().StringVar(&dbPath, "db", "", "path to a CANDB JSON file (or set $CANDB_FILE)")
}
