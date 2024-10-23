#!/bin/bash

# Parse arguments
dry_run=false
if [[ "$1" == "--dry-run" ]]; then
    dry_run=true
fi

# Set variables based on the argument
if $dry_run; then
    message="Validating"
else
    message="Generating"
fi

# Check if the script is being ran in the correct container.
if [ "$(hostname)" != "gerbolyze" ]; then
    echo -e "\e[31mError: This script is meant to be ran in the gerbolyze container.\e[0m"
    exit 1
fi

BASE_DIR=$(git rev-parse --show-toplevel)
FLAPS_SOURCE_DIR=${BASE_DIR}/hardware/module/flaps
FLAPS_BUILD_DIR=${BASE_DIR}/build/flaps

# Try generating the flaps
mkdir -p ${FLAPS_BUILD_DIR}
rm -rf ${FLAPS_BUILD_DIR}/*
echo -e "\033[1;36m$message Flaps\033[0m"
echo -e "\033[96mStep 1: Generate .zip with browser based flap generator\033[0m"
python3 ${FLAPS_SOURCE_DIR}/browser_based_flap_generator/test/browser_based_flap_generator_test.py ${FLAPS_SOURCE_DIR}/browser_based_flap_generator/index.html ${FLAPS_BUILD_DIR}
echo -e "\033[96mStep 2: Convert .zip with gerbolyze\033[0m"
${FLAPS_SOURCE_DIR}/svg_to_gerber.sh ${FLAPS_BUILD_DIR}/flaps.zip ${FLAPS_BUILD_DIR}