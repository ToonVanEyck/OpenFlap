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

# Try generating the flaps
pushd "module/flaps" > /dev/null
echo -e "\033[1;36m$message Flaps\033[0m"
echo -e "\033[96mStep 1: Generate .zip with browser based flap generator\033[0m"
python3 browser_based_flap_generator/test/browser_based_flap_generator_test.py $(realpath browser_based_flap_generator/index.html) $(realpath build/)
echo -e "\033[96mStep 2: Convert .zip with gerbolyze\033[0m"
./svg_to_gerber.sh build/flaps.zip build
popd > /dev/null
