#!/usr/bin/env bash
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASEDIR="$(cd "$SCRIPT_DIR/.." && pwd)"

TEENSY_DIR="$BASEDIR/Firmware/Teensy"
ESP32_DIR="$BASEDIR/Firmware/ESP32"
WEBUI_DIR="$BASEDIR/Firmware/WebUI"
ESP_BOARD="esp32-s3-mini"

RED='\e[1;31m'
GREEN='\e[1;32m'
YELLOW='\e[1;33m'
NC='\e[0m'

TITLE="CarCANdy Build"


export NEWT_COLORS='
    root=white,black
    border=brown,black
    window=lightgray,black
    shadow=gray,black
    title=yellow,black
    button=yellow,brown
    actbutton=white,brown
    compactbutton=brown,black
    checkbox=lightgray,black
    actcheckbox=yellow,brown
    entry=yellow,brown
    disentry=gray,brown
    label=white,black
    listbox=gray,black
    actlistbox=black,brown
    sellistbox=lightgray,green
    actsellistbox=white,brown
    textbox=white,black
    acttextbox=black,cyan
    emptyscale=,gray
    fullscale=,brown
    helpline=white,black
    roottext=lightgrey,black
'
# ---------- helpers ----------

require() {
    command -v "$1" >/dev/null 2>&1 || {
        echo -e "${RED}[ERROR] '$1' not found on PATH.${NC}" >&2
        exit 1
    }
}

# find_port <substring> [<substring> ...] -- first /dev/serial/by-id entry
# matching any of the given substrings (case-insensitive), or empty if none
# present. Mirrors the detection the old per-target scripts did.
find_port() {
    local pattern
    for pattern in "$@"; do
        local match
        match=$(find /dev/serial/by-id -iname "*${pattern}*" 2>/dev/null | head -n1)
        if [[ -n "$match" ]]; then
            echo "$match"
            return 0
        fi
    done
    return 1
}

# Alerts on a failure and asks whether to keep going with the rest of the
# checklist. Returns 0 to continue, 1 to abort the whole run.
confirm_continue_after_failure() {
    whiptail --title "$TITLE" --yesno \
        "$1\n\nSee the terminal output above for details.\n\nContinue with the remaining selected items?" \
        14 70
}

confirm_upload() {
    whiptail --title "$TITLE" --yesno "$1 build succeeded.\n\nUpload now?" 10 60
}

# ---------- per-target build/upload ----------
# Each echoes its own progress (whiptail only owns the screen during the
# dialogs between these calls) and returns a normal exit status.

build_teensy() {
    echo -e "${YELLOW}==> Building Teensy...${NC}"
    (cd "$TEENSY_DIR" && pio run)
}

upload_teensy() {
    local port
    if ! port=$(find_port "Teensy"); then
        whiptail --title "$TITLE" --msgbox "No Teensy found under /dev/serial/by-id -- plug it in and try again." 10 60
        return 1
    fi
    echo -e "${YELLOW}==> Uploading Teensy via ${port}...${NC}"
    (cd "$TEENSY_DIR" && pio run -t upload --upload-port "$port")
}

build_esp32() {
    echo -e "${YELLOW}==> Building ESP32 (${ESP_BOARD})...${NC}"
    (cd "$ESP32_DIR" && pio run -e "$ESP_BOARD")
}

upload_esp32() {
    local port
    # Different boards/drivers have shown up under either identifier in
    # practice (see DevScripts/serial.sh) -- try both.
    if ! port=$(find_port "Espressif" "Arduino"); then
        whiptail --title "$TITLE" --msgbox "No ESP32 found under /dev/serial/by-id -- plug it in and try again." 10 60
        return 1
    fi
    echo -e "${YELLOW}==> Uploading ESP32 firmware via ${port}...${NC}"
    (cd "$ESP32_DIR" && pio run -e "$ESP_BOARD" -t upload --upload-port "$port")
}

build_webui() {
    echo -e "${YELLOW}==> Building WebUI...${NC}"
    (cd "$WEBUI_DIR" && npm run build) || return 1
    echo -e "${YELLOW}==> Copying WebUI build into ESP32/data...${NC}"
    rm -rf "${ESP32_DIR:?}/data"/*
    cp -r "$WEBUI_DIR/dist"/* "$ESP32_DIR/data/"
}

upload_webui() {
    local port
    if ! port=$(find_port "Espressif" "Arduino"); then
        whiptail --title "$TITLE" --msgbox "No ESP32 found under /dev/serial/by-id -- plug it in and try again." 10 60
        return 1
    fi
    echo -e "${YELLOW}==> Uploading WebUI filesystem via ${port}...${NC}"
    (cd "$ESP32_DIR" && pio run -e "$ESP_BOARD" -t uploadfs --upload-port "$port")
}

# Runs build_fn; on success offers upload_fn behind a yes/no prompt. On
# either failing, alerts and asks whether to keep going -- exits the whole
# script if the answer is no.
run_item() {
    local label="$1" build_fn="$2" upload_fn="$3"

    if ! "$build_fn"; then
        echo -e "${RED}[FAILED] $label build failed.${NC}"
        confirm_continue_after_failure "$label build FAILED." || {
            echo "Aborted."
            exit 1
        }
        return
    fi
    echo -e "${GREEN}[OK] $label build succeeded.${NC}"

    confirm_upload "$label" || return
    if ! "$upload_fn"; then
        echo -e "${RED}[FAILED] $label upload failed.${NC}"
        confirm_continue_after_failure "$label upload FAILED." || {
            echo "Aborted."
            exit 1
        }
        return
    fi
    echo -e "${GREEN}[OK] $label upload succeeded.${NC}"
}

# ---------- main ----------

require whiptail
require pio
require npm

selection=$(whiptail --title "$TITLE" --checklist \
    "Select components to build (space to toggle, enter to confirm):" \
    15 70 3 --separate-output \
    "teensy" "Teensy firmware" OFF \
    "esp32" "ESP32 firmware" OFF \
    "webui" "WebUI (built + uploaded to ESP32's filesystem)" OFF \
    3>&1 1>&2 2>&3)
checklist_status=$?
clear

if [[ $checklist_status -ne 0 ]]; then
    echo "Cancelled."
    exit 0
fi
if [[ -z "$selection" ]]; then
    echo "Nothing selected, exiting."
    exit 0
fi

while IFS= read -r item; do
    case "$item" in
    teensy) run_item "Teensy" build_teensy upload_teensy ;;
    esp32) run_item "ESP32" build_esp32 upload_esp32 ;;
    webui) run_item "WebUI" build_webui upload_webui ;;
    esac
done <<<"$selection"

echo -e "${GREEN}All done.${NC}"
