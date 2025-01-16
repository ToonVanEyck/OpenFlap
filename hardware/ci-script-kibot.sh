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
SOURCE_DIR=${BASE_DIR}/hardware
BUILD_DIR=${BASE_DIR}/build/hardware

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

    # Compare title blocks
    grep -A 8 "(title_block" $(realpath $SOURCE_DIR/$path/*.kicad_sch) > /tmp/sch_title_block.txt
    grep -A 8 "(title_block" $(realpath $SOURCE_DIR/$path/*.kicad_pcb) > /tmp/pcb_title_block.txt
    echo "- Comparing Title Blocks"
    if ! diff /tmp/sch_title_block.txt /tmp/pcb_title_block.txt --strip-trailing-cr; then
        echo -e "\e[31mThe title blocks are different.\e[0m"
        all_success=false
    fi

    # Run KiBot
    kibot -c $(realpath $SOURCE_DIR/$path/*.kibot.yaml) -e $(realpath $SOURCE_DIR/$path/*.kicad_sch) -b $(realpath $SOURCE_DIR/$path/*.kicad_pcb) -d $BUILD_DIR/$(basename $(dirname $path)) $kibot_preflight_only
    if [[ $? -ne 0 ]]; then
        all_success=false
    else
        # Copy the generated files to the source directory
        cp $BUILD_DIR/$(basename $(dirname $path))/*.png $SOURCE_DIR/$path/../ > /dev/null 2>&1
        cp $BUILD_DIR/$(basename $(dirname $path))/*.pdf $SOURCE_DIR/$path/../ > /dev/null 2>&1
        cp $BUILD_DIR/$(basename $(dirname $path))/*ibom.html $SOURCE_DIR/$path/../ > /dev/null 2>&1
    fi
done

# Return success only if there were no failures
if $all_success; then
    exit 0
else
    exit 1
fi