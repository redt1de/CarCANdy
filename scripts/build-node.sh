#!/bin/bash
source $(find ../ -type f -name script.env)


echo building NodeJS...
cd $BASEDIR/WebUI/
npm run build
RESULT=$?

if [[ $RESULT -eq 0 ]]; then
        rm -rf ../ESP32/data/*
        cp -r ./dist/* ../ESP32/data/  
else
    echo -e "\n\n${RED}Build FAILED!!!${NC}"
fi

echo -e "${YELLOW}Press Enter to exit... ${NC}";while true; do read -r input;[[ -z "$input" ]] && break;done