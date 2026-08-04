package main

import (
	"fmt"
	"os"
	"text/tabwriter"

	"github.com/spf13/cobra"
)

var listBusFilter string

var listCmd = &cobra.Command{
	Use:   "list",
	Short: "List every documented CAN ID",
	Args:  cobra.NoArgs,
	RunE: func(cmd *cobra.Command, args []string) error {
		d, err := loadDB()
		if err != nil {
			return err
		}

		w := tabwriter.NewWriter(os.Stdout, 0, 2, 2, ' ', 0)
		fmt.Fprintln(w, "ID\tDECIMAL\tVEHICLE BUS\tSIGNALS\tNOTES")
		for _, id := range d.SortedIDs() {
			msg, _ := d.Lookup(id)
			if listBusFilter != "" && !containsBus(msg.VehicleBus, listBusFilter) {
				continue
			}
			notes := msg.Notes
			if len(notes) > 60 {
				notes = notes[:57] + "..."
			}
			fmt.Fprintf(w, "%s\t%d\t%s\t%d\t%s\n",
				formatIDHex(id), msg.IDDecimal, joinOrDash(msg.VehicleBus), len(msg.Signals), notes)
		}
		return w.Flush()
	},
}

func init() {
	listCmd.Flags().StringVar(&listBusFilter, "bus", "", "only list messages whose vehicleBus includes this name (e.g. HS1)")
	rootCmd.AddCommand(listCmd)
}

func containsBus(buses []string, want string) bool {
	for _, b := range buses {
		if b == want {
			return true
		}
	}
	return false
}
