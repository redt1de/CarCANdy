#!/usr/bin/env bash


STEPFILES=(
    "/home/redt1de/make/kicad-lib/Micro USB Plug.step"
    "/home/redt1de/make/kicad-lib/jst-xh-2-5-connector-family-1.snapshot.31/XH_housing_V_steps/XH_housing_2P.step"
    "/home/redt1de/make/kicad-lib/jst-vh-serie-crimp-connectors-1.snapshot.1/stp/VHR-3N.stp"
    "/home/redt1de/make/kicad-lib/jst-xh-2-5-connector-family-1.snapshot.31/XH_housing_V_steps/XH_housing_4P.step"
    "/home/redt1de/make/kicad-lib/jst-xh-2-5-connector-family-1.snapshot.31/XH_housing_V_steps/XH_housing_3P.step"
    "/home/redt1de/make/kicad-lib/jst-vh-serie-crimp-connectors-1.snapshot.1/stp/VHR-2N.stp"
)

HIDE=true

for arg in "$@"; do
    case "$arg" in
        -hide)
            HIDE=true
            ;;
        -show)
            HIDE=false
            ;;
    esac
done




for item in "${STEPFILES[@]}"; do
   if $HIDE; then
        mv "$item"  "$item.HIDE"
    else 
        mv "$item.HIDE"  "$item"
   fi
done
