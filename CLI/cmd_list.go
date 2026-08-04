package main

import (
	"flag"
	"fmt"
	"os"
	"text/tabwriter"
)

func cmdList(args []string) error {
	fs := flag.NewFlagSet("list", flag.ExitOnError)
	dbFlag := addDBFlag(fs)
	busFilter := fs.String("bus", "", "only list messages whose vehicleBus includes this name (e.g. HS1)")
	parseFlags(fs, args)

	d, err := loadDB(*dbFlag)
	if err != nil {
		return err
	}

	w := tabwriter.NewWriter(os.Stdout, 0, 2, 2, ' ', 0)
	fmt.Fprintln(w, "ID\tDECIMAL\tVEHICLE BUS\tSIGNALS\tSOURCES\tNOTES")
	for _, id := range d.SortedIDs() {
		msg, _ := d.Lookup(id)
		if *busFilter != "" && !containsBus(msg.VehicleBus, *busFilter) {
			continue
		}
		notes := msg.Notes
		if len(notes) > 60 {
			notes = notes[:57] + "..."
		}
		fmt.Fprintf(w, "%s\t%d\t%s\t%d\t%s\t%s\n",
			formatIDHex(id), msg.IDDecimal, joinOrDash(msg.VehicleBus), len(msg.Signals), joinOrDash(msg.Sources), notes)
	}
	return w.Flush()
}

func containsBus(buses []string, want string) bool {
	for _, b := range buses {
		if b == want {
			return true
		}
	}
	return false
}
