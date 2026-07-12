#!/bin/bash
RED='\e[1;31m'
GREEN='\e[1;32m'
YELLOW='\e[1;33m'
NC='\e[0m'

echo building Teensy...
cd Teensy/
pio run 
RESULT=$?

if [[ $RESULT -eq 0 ]]; then
    echo -e "\n\n${GREEN}Build succeeded, uploading...\n\n${NC}"
    TEENSY_PORT=$(find /dev/serial/by-id -name '*Teensy*' 2>/dev/null| head -n1)
    if [[ -z "$TEENSY_PORT" ]]; then
        echo -e "\n\n${RED}Teensy not present, aborting upload.\n\n${NC}"
    else 
        pio run -t upload --upload-port $TEENSY_PORT
    fi
    
else
    echo -e "\n\n${RED}Build FAILED!!!${NC}"
fi

while true; do read -r -p "${YELLOW}Press Enter to exit... ${NC}" input;[[ -z "$input" ]] && break;done