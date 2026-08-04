package main

import (
	"fmt"

	"candb/internal/db"

	"github.com/spf13/cobra"
)

var showCmd = &cobra.Command{
	Use:   "show <id>",
	Short: "Show full detail for one CAN ID",
	Args:  cobra.ExactArgs(1),
	RunE: func(cmd *cobra.Command, args []string) error {
		d, err := loadDB()
		if err != nil {
			return err
		}

		id, err := db.ParseID(args[0])
		if err != nil {
			return fmt.Errorf("invalid id %q: %w", args[0], err)
		}
		msg, ok := d.Lookup(id)
		if !ok {
			return fmt.Errorf("no entry for %s (%d) in this database", formatIDHex(id), id)
		}

		fmt.Printf("%s (decimal %d)\n", formatIDHex(id), msg.IDDecimal)
		fmt.Printf("  vehicle bus : %s\n", joinOrDash(msg.VehicleBus))
		if msg.SampleMessage != "" {
			fmt.Printf("  sample      : %s\n", msg.SampleMessage)
		}
		if msg.Notes != "" {
			fmt.Printf("  notes       : %s\n", msg.Notes)
		}

		if len(msg.Signals) == 0 {
			fmt.Println("  (no individual signals documented)")
			return nil
		}

		fmt.Println("  signals:")
		for _, sig := range msg.Signals {
			printSignal(sig)
		}
		return nil
	},
}

func init() {
	rootCmd.AddCommand(showCmd)
}

func printSignal(sig db.Signal) {
	fmt.Printf("  - %s  [bytes=%s, %s]\n", sig.Name, byteIndexStr(sig.ByteIndex), sig.Type)
	if sig.Formula != "" {
		unit := sig.Unit
		if unit == "" {
			unit = "-"
		}
		fmt.Printf("      formula: %s  (unit: %s)\n", sig.Formula, unit)
	}
	if len(sig.Enum) > 0 {
		fmt.Printf("      enum:    %v\n", sig.Enum)
	}
	if len(sig.Bits) > 0 {
		fmt.Printf("      bits:    %v\n", sig.Bits)
	}
	if sig.Notes != "" {
		fmt.Printf("      notes:   %s\n", sig.Notes)
	}
}

func byteIndexStr(idx []int) string {
	if len(idx) == 0 {
		return "-"
	}
	s := ""
	for i, v := range idx {
		if i > 0 {
			s += ","
		}
		s += fmt.Sprintf("%d", v)
	}
	return s
}
