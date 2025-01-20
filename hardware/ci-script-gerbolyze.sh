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
if [ "$(hostname)" != "ecad-mcad" ]; then
    echo -e "\e[31mError: This script is meant to be ran in the ecad-mcad container.\e[0m"
    exit 1
fi

BASE_DIR=$(git rev-parse --show-toplevel)
SOURCE_DIR=${BASE_DIR}/hardware/module/flaps
BUILD_DIR=${BASE_DIR}/build/flaps

# Try generating the flaps
mkdir -p ${BUILD_DIR}
rm -rf ${BUILD_DIR}/*
echo -e "\033[1;36m$message Flaps\033[0m"
echo -e "\033[96mStep 1: Generate .zip with browser based flap generator\033[0m"
python3 ${SOURCE_DIR}/browser_based_flap_generator/test/browser_based_flap_generator_test.py ${SOURCE_DIR}/browser_based_flap_generator/index.html ${BUILD_DIR}
echo -e "\033[96mStep 2: Convert .zip with gerbolyze\033[0m"
${SOURCE_DIR}/svg_to_gerber.sh ${BUILD_DIR}/flaps.zip ${BUILD_DIR}