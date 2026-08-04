// candb is a CLI for looking up CAN IDs/signals in a CarCANdy CANDB JSON
// file and for decoding CarCANdy SD-card log files against it.
package main

import (
	"fmt"
	"os"
)

func main() {
	if err := rootCmd.Execute(); err != nil {
		fmt.Fprintf(os.Stderr, "candb: %v\n", err)
		os.Exit(1)
	}
}
