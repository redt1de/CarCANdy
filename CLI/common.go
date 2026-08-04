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
