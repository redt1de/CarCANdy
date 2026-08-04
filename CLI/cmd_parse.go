package main

import (
	"encoding/csv"
	"flag"
	"fmt"
	"os"
	"sort"
	"strconv"
	"strings"
	"text/tabwriter"

	"candb/internal/canlog"
	"candb/internal/db"
	"candb/internal/decode"
)

func cmdParse(args []string) error {
	fs := flag.NewFlagSet("parse", flag.ExitOnError)
	dbFlag := addDBFlag(fs)
	idFilter := fs.String("id", "", "comma-separated list of CAN IDs to decode (hex or decimal); default: every ID the database documents")
	busFilter := fs.Int("bus", -1, "only decode frames on this CarCANdy bus number (1/2/3); default: all buses")
	minConfidence := fs.String("min-confidence", "", "only show signals at or above this confidence: unconfirmed|community-single-source|community-cross-confirmed|verified-on-vehicle")
	format := fs.String("format", "table", "output format: table|csv")
	showUnknown := fs.Bool("unknown", false, "print a summary (to stderr) of CAN IDs seen in the log that aren't in the database")
	parseFlags(fs, args)

	if fs.NArg() < 1 {
		return fmt.Errorf("usage: candb parse -db <file> <logfile> [flags]")
	}
	logPath := fs.Arg(0)

	d, err := loadDB(*dbFlag)
	if err != nil {
		return err
	}

	var wantIDs map[uint32]bool
	if *idFilter != "" {
		wantIDs = make(map[uint32]bool)
		for _, part := range strings.Split(*idFilter, ",") {
			id, err := db.ParseID(strings.TrimSpace(part))
			if err != nil {
				return fmt.Errorf("invalid -id entry %q: %w", part, err)
			}
			wantIDs[id] = true
		}
	}

	minRank := 0
	if *minConfidence != "" {
		if _, ok := confidenceRank[*minConfidence]; !ok {
			return fmt.Errorf("invalid -min-confidence %q", *minConfidence)
		}
		minRank = confidenceRank[*minConfidence]
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
		ts, bus, id, signal, raw, decoded, confidence string
	}
	var rows []row
	unknown := map[uint32]int{}

	for _, frame := range frames {
		if *busFilter >= 0 && int(frame.Bus) != *busFilter {
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
			if rankOf(sig.Confidence) < minRank {
				continue
			}
			r := decode.Decode(sig, frame.Data)
			decoded := r.Decoded
			raw := fmt.Sprintf("%d", r.RawValue)
			if r.Err != nil {
				decoded = r.Err.Error()
				raw = "-"
			}
			rows = append(rows, row{
				ts:         strconv.FormatUint(uint64(frame.TimestampMs), 10),
				bus:        strconv.FormatUint(uint64(frame.Bus), 10),
				id:         formatIDHex(frame.ID),
				signal:     sig.Name,
				raw:        raw,
				decoded:    decoded,
				confidence: orDash(sig.Confidence),
			})
		}
	}

	switch *format {
	case "csv":
		cw := csv.NewWriter(os.Stdout)
		cw.Write([]string{"timestampMs", "bus", "id", "signal", "raw", "decoded", "confidence"})
		for _, r := range rows {
			cw.Write([]string{r.ts, r.bus, r.id, r.signal, r.raw, r.decoded, r.confidence})
		}
		cw.Flush()
		if err := cw.Error(); err != nil {
			return fmt.Errorf("writing csv: %w", err)
		}
	case "table":
		w := tabwriter.NewWriter(os.Stdout, 0, 2, 2, ' ', 0)
		fmt.Fprintln(w, "TIMESTAMPMS\tBUS\tID\tSIGNAL\tRAW\tDECODED\tCONFIDENCE")
		for _, r := range rows {
			fmt.Fprintf(w, "%s\t%s\t%s\t%s\t%s\t%s\t%s\n", r.ts, r.bus, r.id, r.signal, r.raw, r.decoded, r.confidence)
		}
		if err := w.Flush(); err != nil {
			return err
		}
	default:
		return fmt.Errorf("invalid -format %q, want table or csv", *format)
	}

	if *showUnknown && len(unknown) > 0 {
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
}
