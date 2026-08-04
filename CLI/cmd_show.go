package main

import (
	"flag"
	"fmt"

	"candb/internal/db"
)

func cmdShow(args []string) error {
	fs := flag.NewFlagSet("show", flag.ExitOnError)
	dbFlag := addDBFlag(fs)
	parseFlags(fs, args)

	if fs.NArg() < 1 {
		return fmt.Errorf("usage: candb show -db <file> <id>")
	}

	d, err := loadDB(*dbFlag)
	if err != nil {
		return err
	}

	id, err := db.ParseID(fs.Arg(0))
	if err != nil {
		return fmt.Errorf("invalid id %q: %w", fs.Arg(0), err)
	}
	msg, ok := d.Lookup(id)
	if !ok {
		return fmt.Errorf("no entry for %s (%d) in this database", formatIDHex(id), id)
	}

	fmt.Printf("%s (decimal %d)\n", formatIDHex(id), msg.IDDecimal)
	fmt.Printf("  vehicle bus : %s\n", joinOrDash(msg.VehicleBus))
	if msg.PeriodMs != nil {
		fmt.Printf("  period      : %gms\n", *msg.PeriodMs)
	}
	fmt.Printf("  sources     : %s\n", joinOrDash(msg.Sources))
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
}

func printSignal(sig db.Signal) {
	fmt.Printf("  - %s  [bytes=%s, %s, confidence=%s]\n", sig.Name, byteIndexStr(sig.ByteIndex), sig.Type, orDash(sig.Confidence))
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
	if srcs := sig.Sources(); len(srcs) > 0 {
		fmt.Printf("      source:  %s\n", joinOrDash(srcs))
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

func orDash(s string) string {
	if s == "" {
		return "-"
	}
	return s
}
