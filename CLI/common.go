package main

import (
	"fmt"
	"os"
	"strings"

	"candb/internal/db"
)

// loadDB resolves the --db persistent flag (falling back to $CANDB_FILE)
// and loads the database.
func loadDB() (*db.DB, error) {
	path := dbPath
	if path == "" {
		path = os.Getenv("CANDB_FILE")
	}
	if path == "" {
		return nil, fmt.Errorf("no database file given: pass --db <file> or set $CANDB_FILE")
	}
	return db.Load(path)
}

func formatIDHex(id uint32) string {
	return fmt.Sprintf("0x%03X", id)
}

func joinOrDash(items []string) string {
	if len(items) == 0 {
		return "-"
	}
	return strings.Join(items, ",")
}

// parseIDFilterFlag parses a comma-separated --id flag value (hex or
// decimal entries) into a lookup set. An empty string means "no filter" and
// returns a nil map, matching how both the parse and live commands treat
// the flag's zero value. Shared so the two commands can't drift on how
// --id is parsed.
func parseIDFilterFlag(raw string) (map[uint32]bool, error) {
	if raw == "" {
		return nil, nil
	}
	wantIDs := make(map[uint32]bool)
	for _, part := range strings.Split(raw, ",") {
		id, err := db.ParseID(strings.TrimSpace(part))
		if err != nil {
			return nil, fmt.Errorf("invalid --id entry %q: %w", part, err)
		}
		wantIDs[id] = true
	}
	return wantIDs, nil
}
