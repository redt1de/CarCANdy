// Package canlog parses the CSV log files CarCANdy's Teensy firmware writes
// to SD (see Teensy/src/Logging.cpp): one header line
// "timestampMs,bus,id,extended,rtr,dlc,data" followed by one line per frame,
// with id as a hex string like "0x204" and data as a concatenated hex string
// with no separators (e.g. "01FA0000000000").
package canlog

import (
	"bufio"
	"encoding/hex"
	"fmt"
	"io"
	"strconv"
	"strings"

	"candb/internal/db"
)

type Frame struct {
	Line        int
	TimestampMs uint32
	Bus         uint8
	ID          uint32
	Extended    bool
	RTR         bool
	DLC         uint8
	Data        []byte
}

// Parse reads every well-formed frame line from r. Malformed lines are
// returned via the second value (one entry per bad line, including the line
// number and reason) so callers can report them without aborting the whole
// parse -- a single corrupted line (e.g. from a power-loss mid-write) is
// common on an SD-logged file and shouldn't sink the rest of the log.
func Parse(r io.Reader) ([]Frame, []error) {
	scanner := bufio.NewScanner(r)
	scanner.Buffer(make([]byte, 0, 64*1024), 1024*1024)

	var frames []Frame
	var errs []error
	lineNum := 0

	for scanner.Scan() {
		lineNum++
		line := strings.TrimRight(scanner.Text(), "\r\n")
		if line == "" {
			continue
		}
		fields := strings.Split(line, ",")

		if lineNum == 1 {
			if _, err := strconv.ParseUint(fields[0], 10, 32); err != nil {
				// First field isn't numeric -- this is the header row, skip it.
				continue
			}
		}

		f, err := parseRecord(fields)
		if err != nil {
			errs = append(errs, fmt.Errorf("line %d: %w", lineNum, err))
			continue
		}
		f.Line = lineNum
		frames = append(frames, f)
	}
	if err := scanner.Err(); err != nil {
		errs = append(errs, fmt.Errorf("scanning log: %w", err))
	}
	return frames, errs
}

func parseRecord(fields []string) (Frame, error) {
	if len(fields) < 7 {
		return Frame{}, fmt.Errorf("expected 7 fields (timestampMs,bus,id,extended,rtr,dlc,data), got %d", len(fields))
	}

	ts, err := strconv.ParseUint(strings.TrimSpace(fields[0]), 10, 32)
	if err != nil {
		return Frame{}, fmt.Errorf("bad timestampMs %q: %w", fields[0], err)
	}
	bus, err := strconv.ParseUint(strings.TrimSpace(fields[1]), 10, 8)
	if err != nil {
		return Frame{}, fmt.Errorf("bad bus %q: %w", fields[1], err)
	}
	id, err := db.ParseID(fields[2])
	if err != nil {
		return Frame{}, fmt.Errorf("bad id %q: %w", fields[2], err)
	}
	ext, err := strconv.ParseUint(strings.TrimSpace(fields[3]), 10, 8)
	if err != nil {
		return Frame{}, fmt.Errorf("bad extended flag %q: %w", fields[3], err)
	}
	rtr, err := strconv.ParseUint(strings.TrimSpace(fields[4]), 10, 8)
	if err != nil {
		return Frame{}, fmt.Errorf("bad rtr flag %q: %w", fields[4], err)
	}
	dlc, err := strconv.ParseUint(strings.TrimSpace(fields[5]), 10, 8)
	if err != nil {
		return Frame{}, fmt.Errorf("bad dlc %q: %w", fields[5], err)
	}
	dataHex := strings.TrimSpace(fields[6])
	data, err := hex.DecodeString(dataHex)
	if err != nil {
		return Frame{}, fmt.Errorf("bad data hex %q: %w", dataHex, err)
	}
	if int(dlc) != len(data) {
		return Frame{}, fmt.Errorf("dlc=%d but data has %d bytes", dlc, len(data))
	}

	return Frame{
		TimestampMs: uint32(ts),
		Bus:         uint8(bus),
		ID:          id,
		Extended:    ext != 0,
		RTR:         rtr != 0,
		DLC:         uint8(dlc),
		Data:        data,
	}, nil
}
