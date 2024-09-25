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

# Declare modules
declare -A modules=(
    ["Side Panel"]="module/side_panel/src"
    ["Top Connector"]="top_connector/src"
)

# Run KiBot for each module
for name in "${!modules[@]}"; do
    path=${modules[$name]}
    
    echo -e "\033[1;36m$message $name\033[0m"
    pushd $path > /dev/null
    kibot $kibot_preflight_only
    if [[ $? -ne 0 ]]; then
        all_success=false
    fi
    popd > /dev/null
done

# Return success only if there were no failures
if $all_success; then
    exit 0
else
    exit 1
fi