#!/bin/bash
source $(find ../ -type f -name script.env)

BOARD="esp32-s3-mini"

if [[ -n "$1" ]]; then
    BOARD="${1}"
fi

echo building ESP...
cd $BASEDIR/Firmware/ESP32/
pio run -e esp32-s3-mini
RESULT=$?

if [[ $RESULT -eq 0 ]]; then
   echo -e "\n\n${GREEN}Build succeeded, uploading...\n\n${NC}"
    ESP_PORT=$(find /dev/serial/by-id -name '*Espressif*' 2>/dev/null | head -n1)
    if [[ -z "$ESP_PORT" ]]; then
        echo -e "\n\n${RED}ESP32 not present, aborting upload.\n\n${NC}"
    else 
        # pio run -e "${BOARD}" -t upload --upload-port $ESP_PORT
        # pio run -e "${BOARD}" -t uploadfs --upload-port $ESP_PORT
        pio run -e "${BOARD}" -t upload 

        echo -e "${YELLOW}Press Enter to upload the WebUI... ${NC}";while true; do read -r input;[[ -z "$input" ]] && break;done
        pio run -e "${BOARD}" -t uploadfs 

    fi
    
else
    echo -e "\n\n${RED}Build FAILED!!!${NC}"
fi

echo -e "${YELLOW}Press Enter to exit... ${NC}";while true; do read -r input;[[ -z "$input" ]] && break;done