package main

import (
	"encoding/hex"
	"fmt"
	"os"
	"strings"
	"time"

	"candb/internal/canlog"

	"github.com/spf13/cobra"
	"go.bug.st/serial"
)

var (
	simulatePort      string
	simulateBaud      int
	simulateSpeed     float64
	simulateMaxGapMs  int
	simulateLoop      bool
	simulateIDFilter  string
	simulateBusFilter int
	simulateVerbose   bool
)

var simulateCmd = &cobra.Command{
	Use:   "simulate <logfile>",
	Short: "Replay a CarCANdy SD-card log to a Teensy's Serial port as simulated CAN frames",
	Long: `Reads a CarCANdy CSV log (the same format "candb parse"/"candb live" read)
and replays it to a Teensy running CarCANdy's firmware over its USB Serial
port, pacing sends to match the frames' original recorded timestamps.

The Teensy only accepts these if its config.json has "simulate": true (see
the WebUI's Config page -- off by default, takes effect on next boot). Each
injected frame flows through the exact same pipeline real bus traffic does
(allow/deny filtering, SD/debug logging, ESP32 streaming, plugin dispatch),
so this exercises the real firmware as closely as possible without a car
connected. See Firmware/Teensy/include/Simulator.h for the on-device side
and its wire format.

Gaps between consecutive frames longer than --max-gap are capped (default
2s) so a log with a long idle period (e.g. the car was off) doesn't stall
playback for that long in real time.`,
	Args: cobra.ExactArgs(1),
	RunE: func(cmd *cobra.Command, args []string) error {
		logPath := args[0]

		if simulateSpeed <= 0 {
			return fmt.Errorf("--speed must be > 0")
		}

		wantIDs, err := parseIDFilterFlag(simulateIDFilter)
		if err != nil {
			return err
		}

		f, err := os.Open(logPath)
		if err != nil {
			return fmt.Errorf("opening log file: %w", err)
		}
		frames, parseErrs := canlog.Parse(f)
		f.Close()
		for _, e := range parseErrs {
			fmt.Fprintf(os.Stderr, "candb: warning: %v\n", e)
		}
		if len(frames) == 0 {
			return fmt.Errorf("no frames parsed from %s", logPath)
		}

		var filtered []canlog.Frame
		for _, fr := range frames {
			if simulateBusFilter >= 0 && int(fr.Bus) != simulateBusFilter {
				continue
			}
			if wantIDs != nil && !wantIDs[fr.ID] {
				continue
			}
			filtered = append(filtered, fr)
		}
		if len(filtered) == 0 {
			return fmt.Errorf("no frames left after --bus/--id filtering")
		}

		mode := &serial.Mode{BaudRate: simulateBaud}
		port, err := serial.Open(simulatePort, mode)
		if err != nil {
			return fmt.Errorf("opening serial port %s: %w", simulatePort, err)
		}
		defer port.Close()

		maxGap := time.Duration(simulateMaxGapMs) * time.Millisecond
		sent := 0

		for pass := 0; ; pass++ {
			var prevTs uint32
			for i, fr := range filtered {
				if i > 0 {
					gapMs := int64(fr.TimestampMs) - int64(prevTs)
					if gapMs < 0 {
						// Clock went backward in the log (e.g. a rotation/reset
						// boundary) -- nothing sensible to wait for, send immediately.
						gapMs = 0
					}
					wait := time.Duration(float64(gapMs)/simulateSpeed) * time.Millisecond
					if wait > maxGap {
						wait = maxGap
					}
					if wait > 0 {
						time.Sleep(wait)
					}
				}
				prevTs = fr.TimestampMs

				line := formatSimLine(fr)
				if _, err := port.Write([]byte(line)); err != nil {
					return fmt.Errorf("writing to serial port: %w", err)
				}
				sent++
				if simulateVerbose {
					fmt.Fprintf(os.Stderr, "candb: sent %s", line)
				}
			}
			if !simulateLoop {
				break
			}
			fmt.Fprintf(os.Stderr, "candb: pass %d complete, looping\n", pass+1)
		}

		fmt.Fprintf(os.Stderr, "candb: replayed %d frame(s)\n", sent)
		return nil
	},
}

// formatSimLine matches Simulator.h's wire format exactly: "SIM,<bus>,<id>,
// <extended>,<rtr>,<dlc>,<data>\n", the same columns Logging::logCanFrame()'s
// CSV uses minus the timestamp (the Teensy stamps injected frames with its
// own millis() on arrival, like real RX).
func formatSimLine(fr canlog.Frame) string {
	return fmt.Sprintf("SIM,%d,%s,%d,%d,%d,%s\n",
		fr.Bus,
		formatIDHex(fr.ID),
		boolToBit(fr.Extended),
		boolToBit(fr.RTR),
		fr.DLC,
		strings.ToUpper(hex.EncodeToString(fr.Data)),
	)
}

func boolToBit(b bool) int {
	if b {
		return 1
	}
	return 0
}

func init() {
	simulateCmd.Flags().StringVar(&simulatePort, "port", "", "serial port the CarCANdy Teensy is on (e.g. /dev/ttyACM0, COM3) (required)")
	simulateCmd.Flags().IntVar(&simulateBaud, "baud", 115200, "serial baud rate (matches the Teensy's Serial.begin() call; often ignored by USB-CDC ports)")
	simulateCmd.Flags().Float64Var(&simulateSpeed, "speed", 1.0, "playback speed multiplier (2.0 = twice as fast, 0.5 = half speed)")
	simulateCmd.Flags().IntVar(&simulateMaxGapMs, "max-gap", 2000, "cap on how long (ms) to wait between any two frames, regardless of their recorded gap")
	simulateCmd.Flags().BoolVar(&simulateLoop, "loop", false, "replay the file repeatedly until interrupted (Ctrl+C) instead of once")
	simulateCmd.Flags().StringVar(&simulateIDFilter, "id", "", "comma-separated list of CAN IDs to replay (hex or decimal); default: every frame in the file")
	simulateCmd.Flags().IntVar(&simulateBusFilter, "bus", -1, "only replay frames on this CarCANdy bus number (1/2/3); default: all buses")
	simulateCmd.Flags().BoolVar(&simulateVerbose, "verbose", false, "print each frame sent (to stderr) as it's sent")
	simulateCmd.MarkFlagRequired("port")
	rootCmd.AddCommand(simulateCmd)
}
