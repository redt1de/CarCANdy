package main

import (
	"encoding/csv"
	"fmt"
	"os"
	"sort"
	"strconv"
	"text/tabwriter"

	"candb/internal/canlog"
	"candb/internal/decode"

	"github.com/spf13/cobra"
)

var (
	parseIDFilter    string
	parseBusFilter   int
	parseFormat      string
	parseShowUnknown bool
)

var parseCmd = &cobra.Command{
	Use:   "parse <logfile>",
	Short: "Parse a CarCANdy SD-card log and decode it against the database",
	Args:  cobra.ExactArgs(1),
	RunE: func(cmd *cobra.Command, args []string) error {
		logPath := args[0]

		d, err := loadDB()
		if err != nil {
			return err
		}

		wantIDs, err := parseIDFilterFlag(parseIDFilter)
		if err != nil {
			return err
		}

		f, err := os.Open(logPath)
		if err != nil {
			return fmt.Errorf("opening log file: %w", err)
		}
		defer f.Close()

		frames, parseErrs := canlog.Parse(f)
		for _, e := range parseErrs {
			fmt.Fprintf(os.Stderr, "candb: warning: %v\n", e)
		}

		type row struct {
			ts, bus, id, signal, raw, decoded string
		}
		var rows []row
		unknown := map[uint32]int{}

		for _, frame := range frames {
			if parseBusFilter >= 0 && int(frame.Bus) != parseBusFilter {
				continue
			}
			if wantIDs != nil && !wantIDs[frame.ID] {
				continue
			}
			msg, ok := d.Lookup(frame.ID)
			if !ok {
				unknown[frame.ID]++
				continue
			}
			for _, sig := range msg.Signals {
				r := decode.Decode(sig, frame.Data)
				decoded := r.Decoded
				raw := fmt.Sprintf("%d", r.RawValue)
				if r.Err != nil {
					decoded = r.Err.Error()
					raw = "-"
				}
				rows = append(rows, row{
					ts:      strconv.FormatUint(uint64(frame.TimestampMs), 10),
					bus:     strconv.FormatUint(uint64(frame.Bus), 10),
					id:      formatIDHex(frame.ID),
					signal:  sig.Name,
					raw:     raw,
					decoded: decoded,
				})
			}
		}

		switch parseFormat {
		case "csv":
			cw := csv.NewWriter(os.Stdout)
			cw.Write([]string{"timestampMs", "bus", "id", "signal", "raw", "decoded"})
			for _, r := range rows {
				cw.Write([]string{r.ts, r.bus, r.id, r.signal, r.raw, r.decoded})
			}
			cw.Flush()
			if err := cw.Error(); err != nil {
				return fmt.Errorf("writing csv: %w", err)
			}
		case "table":
			w := tabwriter.NewWriter(os.Stdout, 0, 2, 2, ' ', 0)
			fmt.Fprintln(w, "TIMESTAMPMS\tBUS\tID\tSIGNAL\tRAW\tDECODED")
			for _, r := range rows {
				fmt.Fprintf(w, "%s\t%s\t%s\t%s\t%s\t%s\n", r.ts, r.bus, r.id, r.signal, r.raw, r.decoded)
			}
			if err := w.Flush(); err != nil {
				return err
			}
		default:
			return fmt.Errorf("invalid --format %q, want table or csv", parseFormat)
		}

		if parseShowUnknown && len(unknown) > 0 {
			type idCount struct {
				id    uint32
				count int
			}
			ids := make([]idCount, 0, len(unknown))
			for id, c := range unknown {
				ids = append(ids, idCount{id, c})
			}
			sort.Slice(ids, func(i, j int) bool { return ids[i].count > ids[j].count })
			fmt.Fprintln(os.Stderr, "\nIDs seen in log but not in database:")
			for _, ic := range ids {
				fmt.Fprintf(os.Stderr, "  %s  x%d\n", formatIDHex(ic.id), ic.count)
			}
		}

		return nil
	},
}

func init() {
	parseCmd.Flags().StringVar(&parseIDFilter, "id", "", "comma-separated list of CAN IDs to decode (hex or decimal); default: every ID the database documents")
	parseCmd.Flags().IntVar(&parseBusFilter, "bus", -1, "only decode frames on this CarCANdy bus number (1/2/3); default: all buses")
	parseCmd.Flags().StringVar(&parseFormat, "format", "table", "output format: table|csv")
	parseCmd.Flags().BoolVar(&parseShowUnknown, "unknown", false, "print a summary (to stderr) of CAN IDs seen in the log that aren't in the database")
	rootCmd.AddCommand(parseCmd)
}
