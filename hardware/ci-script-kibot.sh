#!/bin/bash

# Parse arguments
dry_run=false
if [[ "$1" == "--dry-run" ]]; then
    dry_run=true
fi

# Set variables based on the argument
if $dry_run; then
    message="Validating"
    kibot_preflight_only="-i"
else
    message="Generating"
    kibot_preflight_only=""
fi

# Check if the script is being ran in the correct container.
if [ "$(hostname)" != "kibot" ]; then
    echo -e "\e[31mError: This script is meant to be ran in the gerbolyze container.\e[0m"
    exit 1
fi

BASE_DIR=$(git rev-parse --show-toplevel)
HW_SOURCE_DIR=${BASE_DIR}/hardware
HW_BUILD_DIR=${BASE_DIR}/build/hardware

# Declare modules
declare -A modules=(
    ["Side Panel"]="module/side_panel/src"
    ["Encoder Wheel"]="module/encoder_wheel/src"
    ["Top Connector"]="top_connector/src"
    ["Controller"]="controller/src"
)

# Run KiBot for each module
for name in "${!modules[@]}"; do
    path=${modules[$name]}
    
    echo -e "\033[1;36m$message $name\033[0m"
    kibot -c $(realpath $HW_SOURCE_DIR/$path/*.kibot.yaml) -e $(realpath $HW_SOURCE_DIR/$path/*.kicad_sch) -b $(realpath $HW_SOURCE_DIR/$path/*.kicad_pcb) -d $HW_BUILD_DIR/$(basename $(dirname $path))
    if [[ $? -ne 0 ]]; then
        all_success=false
    else
        # Copy the generated files to the source directory
        cp $HW_BUILD_DIR/$(basename $(dirname $path))/*.png $HW_SOURCE_DIR/$path/../
        cp $HW_BUILD_DIR/$(basename $(dirname $path))/*.pdf $HW_SOURCE_DIR/$path/../
        cp $HW_BUILD_DIR/$(basename $(dirname $path))/*ibom.html $HW_SOURCE_DIR/$path/../
    fi
done

# Return success only if there were no failures
if $all_success; then
    exit 0
else
    exit 1
fi