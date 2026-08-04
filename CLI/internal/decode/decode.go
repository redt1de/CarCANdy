// Package decode turns a CANDB Signal definition plus a raw CAN frame payload
// into a human-readable value.
package decode

import (
	"fmt"
	"go/ast"
	"go/parser"
	"go/token"
	"sort"
	"strconv"
	"strings"

	"candb/internal/db"
)

// Result is the outcome of decoding one signal against one frame's data.
type Result struct {
	Signal   db.Signal
	RawValue uint64
	Numeric  *float64 // set when Signal.Formula evaluated successfully
	Decoded  string   // human-readable summary, always set unless Err != nil
	Err      error
}

// Decode evaluates a single signal against a frame's data bytes.
func Decode(sig db.Signal, data []byte) Result {
	res := Result{Signal: sig}

	raw, ok := rawValue(sig, data)
	if !ok {
		res.Err = fmt.Errorf("signal %q references byte index beyond frame dlc=%d", sig.Name, len(data))
		return res
	}
	res.RawValue = raw

	var parts []string

	if sig.Formula != "" {
		byteVals := make(map[int]uint64, len(data))
		for i, b := range data {
			byteVals[i] = uint64(b)
		}
		if v, err := evalFormula(sig.Formula, byteVals); err == nil {
			res.Numeric = &v
			if sig.Unit != "" {
				parts = append(parts, fmt.Sprintf("%.4g %s", v, sig.Unit))
			} else {
				parts = append(parts, fmt.Sprintf("%.4g", v))
			}
		} else {
			parts = append(parts, fmt.Sprintf("formula error: %v", err))
		}
	}

	if len(sig.Enum) > 0 {
		if desc, ok := sig.Enum[strconv.FormatUint(raw, 10)]; ok {
			parts = append(parts, desc)
		} else {
			parts = append(parts, fmt.Sprintf("unmapped enum value %d (0x%X)", raw, raw))
		}
	}

	if len(sig.Bits) > 0 {
		keys := make([]int, 0, len(sig.Bits))
		for k := range sig.Bits {
			if n, err := strconv.Atoi(k); err == nil {
				keys = append(keys, n)
			}
		}
		sort.Ints(keys)
		var flags []string
		for _, bit := range keys {
			if raw&(1<<uint(bit)) != 0 {
				flags = append(flags, sig.Bits[strconv.Itoa(bit)])
			}
		}
		if len(flags) == 0 {
			parts = append(parts, "no bits set")
		} else {
			parts = append(parts, strings.Join(flags, "+"))
		}
	}

	if len(parts) == 0 {
		parts = append(parts, fmt.Sprintf("raw=%d (0x%X)", raw, raw))
	}
	res.Decoded = strings.Join(parts, "; ")
	return res
}

// rawValue concatenates the bytes at sig.ByteIndex, most-significant first,
// into a single integer -- e.g. byteIndex [3,4] with data[3]=0x01,data[4]=0x7D
// yields 0x017D.
func rawValue(sig db.Signal, data []byte) (uint64, bool) {
	var v uint64
	for _, idx := range sig.ByteIndex {
		if idx < 0 || idx >= len(data) {
			return 0, false
		}
		v = (v << 8) | uint64(data[idx])
	}
	return v, true
}

// evalFormula evaluates formulas written in the CANDB schema's convention
// (e.g. "((b5<<8)+b6)/100"), where bN refers to byteVals[N]. It supports the
// arithmetic and bitwise operators Go's own expression grammar provides,
// which happens to cover every formula style used in the CANDB files, so we
// reuse go/parser instead of hand-rolling an expression parser.
func evalFormula(formula string, byteVals map[int]uint64) (float64, error) {
	expr, err := parser.ParseExpr(formula)
	if err != nil {
		return 0, fmt.Errorf("invalid formula %q: %w", formula, err)
	}
	return evalExpr(expr, byteVals)
}

func evalExpr(e ast.Expr, byteVals map[int]uint64) (float64, error) {
	switch n := e.(type) {
	case *ast.ParenExpr:
		return evalExpr(n.X, byteVals)

	case *ast.BasicLit:
		v, err := strconv.ParseFloat(n.Value, 64)
		if err != nil {
			return 0, fmt.Errorf("invalid literal %q: %w", n.Value, err)
		}
		return v, nil

	case *ast.Ident:
		if len(n.Name) >= 2 && n.Name[0] == 'b' {
			if idx, err := strconv.Atoi(n.Name[1:]); err == nil {
				if v, ok := byteVals[idx]; ok {
					return float64(v), nil
				}
				return 0, fmt.Errorf("byte index b%d not present in this frame", idx)
			}
		}
		return 0, fmt.Errorf("unknown identifier %q in formula", n.Name)

	case *ast.UnaryExpr:
		v, err := evalExpr(n.X, byteVals)
		if err != nil {
			return 0, err
		}
		if n.Op == token.SUB {
			return -v, nil
		}
		return 0, fmt.Errorf("unsupported unary operator %v", n.Op)

	case *ast.BinaryExpr:
		lv, err := evalExpr(n.X, byteVals)
		if err != nil {
			return 0, err
		}
		rv, err := evalExpr(n.Y, byteVals)
		if err != nil {
			return 0, err
		}
		switch n.Op {
		case token.ADD:
			return lv + rv, nil
		case token.SUB:
			return lv - rv, nil
		case token.MUL:
			return lv * rv, nil
		case token.QUO:
			if rv == 0 {
				return 0, fmt.Errorf("division by zero")
			}
			return lv / rv, nil
		case token.AND:
			return float64(int64(lv) & int64(rv)), nil
		case token.OR:
			return float64(int64(lv) | int64(rv)), nil
		case token.XOR:
			return float64(int64(lv) ^ int64(rv)), nil
		case token.SHL:
			return float64(int64(lv) << uint(int64(rv))), nil
		case token.SHR:
			return float64(int64(lv) >> uint(int64(rv))), nil
		default:
			return 0, fmt.Errorf("unsupported operator %v", n.Op)
		}

	default:
		return 0, fmt.Errorf("unsupported expression syntax: %T", e)
	}
}
