#!/bin/bash
source $(find ../ -type f -name script.env)


echo building Teensy...
cd $BASEDIR/Firmware/Teensy/
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

echo -e "${YELLOW}Press Enter to exit... ${NC}";while true; do read -r input;[[ -z "$input" ]] && break;done