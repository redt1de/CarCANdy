#!/usr/bin/env bash

set -euo pipefail

SESSION="serial"
BAUD=115200
# ESP_PORT=$(find /dev/serial/by-id -name '*Espressif*' | head -n1)
ESP_PORT=$(find /dev/serial/by-id -name '*Arduino*' | head -n1)
TEENSY_PORT=$(find /dev/serial/by-id -name '*Teensy*' | head -n1)


tmux kill-session -t "$SESSION" 2>/dev/null || true

tmux new-session -d -s "$SESSION" "pio device monitor -p $ESP_PORT -b $BAUD"

tmux split-window -h -t "$SESSION:0" "pio device monitor -p $TEENSY_PORT -b $BAUD"

tmux select-layout -t "$SESSION:0" even-horizontal
tmux set-option -t "$SESSION" pane-border-status top
tmux select-pane -t "$SESSION:0.1" -T "ESP32"
tmux select-pane -t "$SESSION:0.0" -T "Teensy"
tmux select-pane -t "$SESSION:0.0"
exec tmux attach -t "$SESSION"