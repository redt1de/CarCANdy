#!/usr/bin/env bash
# Extract RPM from a raw 0x204 frame (Ford S550 HS-CAN, per DOC/CANDB/2015_ford_mustang_gt.json)
# Formula: rpm = (b3*256 + b4) * 2   -- verified against this project's own vehicle
#          (b3/b4 are 0-indexed bytes, so the 4th and 5th space-separated hex pairs)


FRAME="C0 00 7D 01 E9 00 00 00"

read -ra BYTES <<< "$FRAME"
B3=$((16#${BYTES[3]}))
B4=$((16#${BYTES[4]}))
RPM=$(( (B3 * 256 + B4) * 2 ))
echo "RPM: $RPM"

