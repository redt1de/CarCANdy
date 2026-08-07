package main

import (
	"bufio"
	"encoding/csv"
	"fmt"
	"os"
	"strconv"
	"strings"

	"candb/internal/canlog"
	"candb/internal/decode"

	"github.com/spf13/cobra"
	"go.bug.st/serial"
)

var (
	livePort        string
	liveBaud        int
	liveIDFilter    string
	liveBusFilter   int
	liveFormat      string
	liveShowUnknown bool
)

var liveCmd = &cobra.Command{
	Use:   "live",
	Short: "Stream and decode live CAN frames from a CarCANdy device's serial port",
	Long: `Reads a CarCANdy Teensy's Serial output directly (a USB-CDC port, e.g.
/dev/ttyACM0 on Linux or COM3 on Windows) and decodes "[CAN] ..." lines
against the database as they arrive -- the same CSV format and decode path
"candb parse" uses for SD log files, just streamed live instead of read from
a finished file. Runs until interrupted (Ctrl+C) or the port closes.

Non-CAN lines on the same Serial stream (e.g. "[INFO]"/"[Logging]"/
"[CANManager]" diagnostics) are passed through to stderr rather than
dropped, so device boot/status messages stay visible.`,
	Args: cobra.NoArgs,
	RunE: func(cmd *cobra.Command, args []string) error {
		d, err := loadDB()
		if err != nil {
			return err
		}

		wantIDs, err := parseIDFilterFlag(liveIDFilter)
		if err != nil {
			return err
		}

		if liveFormat != "table" && liveFormat != "csv" {
			return fmt.Errorf("invalid --format %q, want table or csv", liveFormat)
		}

		mode := &serial.Mode{BaudRate: liveBaud}
		port, err := serial.Open(livePort, mode)
		if err != nil {
			return fmt.Errorf("opening serial port %s: %w", livePort, err)
		}
		defer port.Close()

		var csvWriter *csv.Writer
		if liveFormat == "csv" {
			csvWriter = csv.NewWriter(os.Stdout)
			csvWriter.Write([]string{"timestampMs", "bus", "id", "signal", "raw", "decoded"})
			csvWriter.Flush()
		} else {
			fmt.Printf("%-12s %-4s %-8s %-24s %-14s %s\n", "TIMESTAMPMS", "BUS", "ID", "SIGNAL", "RAW", "DECODED")
		}

		scanner := bufio.NewScanner(port)
		lineNum := 0
		for scanner.Scan() {
			lineNum++
			line := strings.TrimRight(scanner.Text(), "\r\n")

			rest, ok := strings.CutPrefix(line, "[CAN] ")
			if !ok {
				if strings.TrimSpace(line) != "" {
					fmt.Fprintf(os.Stderr, "candb: device: %s\n", line)
				}
				continue
			}

			frame, err := canlog.ParseRecord(strings.Split(rest, ","))
			if err != nil {
				fmt.Fprintf(os.Stderr, "candb: warning: line %d: %v\n", lineNum, err)
				continue
			}

			if liveBusFilter >= 0 && int(frame.Bus) != liveBusFilter {
				continue
			}
			if wantIDs != nil && !wantIDs[frame.ID] {
				continue
			}

			msg, ok := d.Lookup(frame.ID)
			if !ok {
				if liveShowUnknown {
					fmt.Fprintf(os.Stderr, "candb: unknown id %s\n", formatIDHex(frame.ID))
				}
				continue
			}

			ts := strconv.FormatUint(uint64(frame.TimestampMs), 10)
			bus := strconv.FormatUint(uint64(frame.Bus), 10)
			id := formatIDHex(frame.ID)

			for _, sig := range msg.Signals {
				r := decode.Decode(sig, frame.Data)
				decoded := r.Decoded
				raw := fmt.Sprintf("%d", r.RawValue)
				if r.Err != nil {
					decoded = r.Err.Error()
					raw = "-"
				}

				if liveFormat == "csv" {
					csvWriter.Write([]string{ts, bus, id, sig.Name, raw, decoded})
					csvWriter.Flush()
				} else {
					fmt.Printf("%-12s %-4s %-8s %-24s %-14s %s\n", ts, bus, id, sig.Name, raw, decoded)
				}
			}
		}
		if err := scanner.Err(); err != nil {
			return fmt.Errorf("reading serial port: %w", err)
		}
		return nil
	},
}

func init() {
	liveCmd.Flags().StringVar(&livePort, "port", "", "serial port the CarCANdy Teensy is on (e.g. /dev/ttyACM0, COM3) (required)")
	liveCmd.Flags().IntVar(&liveBaud, "baud", 115200, "serial baud rate (matches the Teensy's Serial.begin() call; often ignored by USB-CDC ports)")
	liveCmd.Flags().StringVar(&liveIDFilter, "id", "", "comma-separated list of CAN IDs to decode (hex or decimal); default: every ID the database documents")
	liveCmd.Flags().IntVar(&liveBusFilter, "bus", -1, "only decode frames on this CarCANdy bus number (1/2/3); default: all buses")
	liveCmd.Flags().StringVar(&liveFormat, "format", "table", "output format: table|csv")
	liveCmd.Flags().BoolVar(&liveShowUnknown, "unknown", false, "print (to stderr) each CAN ID seen that isn't in the database, as it's seen")
	liveCmd.MarkFlagRequired("port")
	rootCmd.AddCommand(liveCmd)
}
