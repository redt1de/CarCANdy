# OBDHacker CAN Database Schema v1.0

A CAN database consists of an array of CAN message objects.

```json
[
    {
        ...
    }
]
```

---

## CAN Message

| Field | Type | Required | Description |
|---------|------|----------|-------------|
| id_dec | integer | Yes | CAN ID in decimal |
| id_hex | string | Yes | CAN ID in hexadecimal (0x123) |
| bus | string | Yes | Bus identifier (HS1, HS2, MS, etc.) |
| signals | array | Yes | List of decoded signals contained in this message |

Example:

```json
{
    "id_dec": 560,
    "id_hex": "0x230",
    "bus": "HS1",
    "signals": []
}
```

---

# Signal

Each CAN message contains one or more signals.

| Field | Type | Required | Description |
|---------|------|----------|-------------|
| name | string | Yes | Human-readable signal name |
| type | string | Yes | Decoding type |
| byte | integer | Usually | Byte containing the value |
| bytes | array | Optional | Multiple-byte value |
| enumeration | object | Optional | Enum lookup table |
| range | object | Optional | Numeric ranges |
| notes | string | Optional | Comments, formulas, uncertainties |

Exactly one of `byte` or `bytes` should be present.

---

# Supported Types

## enum

A byte whose numeric value corresponds to a lookup table.

```json
{
    "name": "Gear Position",
    "byte": 2,
    "type": "enum",
    "enumeration": {
        "0": "Park",
        "2": "Reverse",
        "4": "Neutral",
        "6": "Drive",
        "8": "Sport"
    }
}
```

---

## range

A value that falls within documented ranges.

```json
{
    "name": "Instrument Brightness",
    "byte": 4,
    "type": "range",
    "range": {
        "lights_off": "1-12",
        "lights_on": "13-18"
    }
}
```

---

## unknown

Used when a signal is known to exist but its decoding is incomplete.

```json
{
    "name": "Engine RPM",
    "bytes": [7,8],
    "type": "unknown",
    "notes": "Formula unknown"
}
```

---

# Future Types

Additional decoding types may be added in future versions without changing the overall schema.

Examples include:

- bool
- bitfield
- uint
- int
- float
- ascii
- string
- raw

Applications should ignore unknown types they do not support.

---

# Example Database

```json
[
    {
        "id_dec": 560,
        "id_hex": "0x230",
        "bus": "HS1",
        "signals": [
            {
                "name": "Gear Position",
                "byte": 2,
                "type": "enum",
                "enumeration": {
                    "0": "Park",
                    "2": "Reverse",
                    "4": "Neutral",
                    "6": "Drive",
                    "8": "Sport"
                }
            }
        ]
    }
]
```