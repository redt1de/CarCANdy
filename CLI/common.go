package main

import (
	"flag"
	"fmt"
	"os"
	"strings"

	"candb/internal/db"
)

// confidenceRank orders CANDB confidence levels from least to most trusted,
// for -min-confidence filtering. "conflict" sits alongside "unconfirmed":
// it means two sources disagree, which is exactly as unproven as a single
// unverified guess until you check it against your own car.
var confidenceRank = map[string]int{
	"unconfirmed":               0,
	"conflict":                  0,
	"community-single-source":   1,
	"community-cross-confirmed": 2,
	"verified-on-vehicle":       3,
}

func rankOf(confidence string) int {
	if r, ok := confidenceRank[confidence]; ok {
		return r
	}
	return 0
}

// addDBFlag registers the -db flag shared by every subcommand and returns a
// getter that resolves it (falling back to $CANDB_FILE) once flags are parsed.
func addDBFlag(fs *flag.FlagSet) *string {
	return fs.String("db", "", "path to a CANDB JSON file (or set $CANDB_FILE)")
}

func loadDB(dbPath string) (*db.DB, error) {
	if dbPath == "" {
		dbPath = os.Getenv("CANDB_FILE")
	}
	if dbPath == "" {
		return nil, fmt.Errorf("no database file given: pass -db <file> or set $CANDB_FILE")
	}
	return db.Load(dbPath)
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

// parseFlags parses args against fs after reordering them so flags can
// appear before or after positional arguments. Go's flag package stops
// parsing at the first non-flag token, so e.g. "parse logfile.csv -bus 3"
// would otherwise silently leave -bus unparsed instead of erroring --
// reordering avoids that trap entirely rather than relying on users to
// always put the logfile/id last.
func parseFlags(fs *flag.FlagSet, args []string) {
	boolFlags := map[string]bool{}
	fs.VisitAll(func(f *flag.Flag) {
		if bv, ok := f.Value.(interface{ IsBoolFlag() bool }); ok && bv.IsBoolFlag() {
			boolFlags[f.Name] = true
		}
	})

	var flagArgs, positional []string
	for i := 0; i < len(args); i++ {
		a := args[i]
		if a == "--" {
			positional = append(positional, args[i+1:]...)
			break
		}
		if len(a) > 1 && a[0] == '-' {
			name := strings.TrimLeft(a, "-")
			flagArgs = append(flagArgs, a)
			if strings.ContainsRune(name, '=') || boolFlags[strings.SplitN(name, "=", 2)[0]] {
				continue
			}
			if i+1 < len(args) {
				i++
				flagArgs = append(flagArgs, args[i])
			}
			continue
		}
		positional = append(positional, a)
	}
	fs.Parse(append(flagArgs, positional...))
}
