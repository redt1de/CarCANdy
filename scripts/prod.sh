#!/bin/bash
source $(find ../ -type f -name script.env)

function errfail(){
    echo "[ERROR] $1"
    exit 1
}

echo "[*] buiding webui..."
rm -rf ESP32/data/*
rm -rf WebUI/dist/*
cd $BASEDIR/WebUI
npm run build || errfail "webui build failed"
cp -r dist/* ../ESP32/data/
cd ..

echo "[*] buiding teensy firmware..."
cd $BASEDIR/Teensy/
pio run || errfail "teensy firmware build failed"

echo "[*] uploading to teensy..."
pio run -t upload || errfail "upload to teensy failed"


echo "[*] buiding ESP32 firmware..."
cd $BASEDIR/ESP32
pio run || errfail "esp32 firmware build failed"

echo "[*] uploading to ESP32..."
pio run -t upload || errfail "upload to ESP32 failed"

echo "[*] uploading WebUI to ESP32..."
pio run -t uploadfs ||errfail "webui upload failed"


echo "[+] Succeeded!

