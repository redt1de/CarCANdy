// Package db loads and indexes a CarCANdy CANDB JSON file (see DOC/CANDB/README.md
// for the schema this mirrors).
package db

import (
	"encoding/json"
	"fmt"
	"os"
	"sort"
	"strconv"
	"strings"
)

type Vehicle struct {
	Make  string `json:"make"`
	Model string `json:"model"`
	Years string `json:"years"`
	Notes string `json:"notes"`
}

type Signal struct {
	Name      string            `json:"name"`
	ByteIndex []int             `json:"byteIndex"`
	Type      string            `json:"type"`
	Formula   string            `json:"formula,omitempty"`
	Unit      string            `json:"unit,omitempty"`
	Enum      map[string]string `json:"enum,omitempty"`
	Bits      map[string]string `json:"bits,omitempty"`
	SourceRaw any               `json:"sourceRaw,omitempty"`
	Notes     string            `json:"notes,omitempty"`
}

type Message struct {
	IDDecimal     int      `json:"idDecimal"`
	VehicleBus    []string `json:"vehicleBus"`
	Signals       []Signal `json:"signals"`
	Notes         string   `json:"notes,omitempty"`
	SampleMessage string   `json:"sampleMessage,omitempty"`
}

type DB struct {
	Vehicle  Vehicle            `json:"vehicle"`
	Messages map[string]Message `json:"messages"`

	// byID maps the numeric CAN ID to the original key used in Messages
	// (e.g. "0x076"), so lookups don't depend on the caller reproducing the
	// exact zero-padding/hex-case of the JSON key.
	byID map[uint32]string
}

// Load reads and indexes a CANDB JSON file.
func Load(path string) (*DB, error) {
	raw, err := os.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("reading db file: %w", err)
	}
	var d DB
	if err := json.Unmarshal(raw, &d); err != nil {
		return nil, fmt.Errorf("parsing db file %s: %w", path, err)
	}
	d.byID = make(map[uint32]string, len(d.Messages))
	for key := range d.Messages {
		id, err := ParseID(key)
		if err != nil {
			return nil, fmt.Errorf("db file %s: message key %q is not a valid CAN ID: %w", path, key, err)
		}
		d.byID[id] = key
	}
	return &d, nil
}

// ParseID parses a CAN ID given as "0x204"/"0X204" (hex) or "516" (decimal).
func ParseID(s string) (uint32, error) {
	s = strings.TrimSpace(s)
	if strings.HasPrefix(strings.ToLower(s), "0x") {
		v, err := strconv.ParseUint(s[2:], 16, 32)
		return uint32(v), err
	}
	v, err := strconv.ParseUint(s, 10, 32)
	return uint32(v), err
}

// Lookup returns the message for a numeric CAN ID, if the database has one.
func (d *DB) Lookup(id uint32) (Message, bool) {
	key, ok := d.byID[id]
	if !ok {
		return Message{}, false
	}
	return d.Messages[key], true
}

// SortedIDs returns every documented CAN ID in ascending numeric order.
func (d *DB) SortedIDs() []uint32 {
	ids := make([]uint32, 0, len(d.byID))
	for id := range d.byID {
		ids = append(ids, id)
	}
	sort.Slice(ids, func(i, j int) bool { return ids[i] < ids[j] })
	return ids
}
