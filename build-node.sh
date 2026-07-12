#!/bin/bash
RED='\e[1;31m'
GREEN='\e[1;32m'
YELLOW='\e[1;33m'
NC='\e[0m'

echo building NodeJS...
cd WebUI/
npm run build
RESULT=$?

if [[ $RESULT -eq 0 ]]; then
    echo -e "\n\n${YELLOW}Build succeeded, ready to upload, Press Enter once ESP32 build is complete......\n\n${NC}"
    while true; do read -r -p "" input;[[ -z "$input" ]] && break;done

    ESP_PORT=$(find /dev/serial/by-id -name '*Espressif*' 2>/dev/null | head -n1)
    if [[ -z "$ESP_PORT" ]]; then
        echo -e "\n\n${RED}ESP32 not present, aborting upload.\n\n${NC}"
    else 
        rm -rf ../ESP32/data/*
        cp -r ./dist/* ../ESP32/data/
        cd ../ESP32/
        pio run -e esp32-s3-mini -t uploadfs --upload-port $ESP_PORT

    fi   
else
    echo -e "\n\n${RED}Build FAILED!!!${NC}"
fi
while true; do read -r -p "${YELLOW}Press Enter to exit... ${NC}" input;[[ -z "$input" ]] && break;done