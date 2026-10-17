#!/bin/bash
source $(find ../ -type f -name script.env)

function errfail(){
    echo "[ERROR] $1"
    exit 1
}

echo "[*] buiding webui..."
rm -rf $BASEDIR/Firmware/ESP32/data/*
rm -rf $BASEDIR/Firmware/WebUI/dist/*
cd $BASEDIR/Firmware/WebUI
npm run build || errfail "webui build failed"
cp -r dist/* ../ESP32/data/
cd $BASEDIR

echo "[*] buiding teensy firmware..."
cd $BASEDIR/Firmware/Teensy/
pio run || errfail "teensy firmware build failed"

echo "[*] uploading to teensy..."
pio run -t upload || errfail "upload to teensy failed"


echo "[*] buiding ESP32 firmware..."
cd $BASEDIR/Firmware/ESP32
pio run || errfail "esp32 firmware build failed"

echo "[*] uploading to ESP32..."
pio run -t upload || errfail "upload to ESP32 failed"

echo "[*] uploading WebUI to ESP32..."
pio run -t uploadfs ||errfail "webui upload failed"


echo "[+] Succeeded!"
