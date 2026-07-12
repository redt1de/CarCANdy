#!/usr/bin/env bash

# set -euo pipefail

SESSION="build"
BAUD=115200

tmux kill-session -t "$SESSION" 2>/dev/null || true
tmux new-session -d -s "$SESSION" "./build-teensy.sh"
tmux split-window -v -t "$SESSION:0" "./build-esp.sh"
tmux split-window -v -t "$SESSION:0.1" "./build-node.sh"
tmux select-layout -t "$SESSION:0" even-vertical
tmux set-option -t "$SESSION" pane-border-status top
tmux select-pane -t "$SESSION:0.0" -T "Teensy"
tmux select-pane -t "$SESSION:0.1" -T "ESP32"
tmux select-pane -t "$SESSION:0.2" -T "NodeJS"
# tmux select-pane -t "$SESSION:0.0"
exec tmux attach -t "$SESSION"

