#!/bin/bash

# Parse arguments
dry_run=""
if [[ "$1" == "--dry-run" ]]; then
    dry_run="--dry-run"
fi

# Check if the script is being ran in the correct container.
if [ "$(hostname)" != "freecad" ]; then
    echo -e "\e[31mError: This script is meant to be ran in the freecad container.\e[0m"
    exit 1
fi

# Declare FreeCAD model
model="openflap_cad_model.FCStd"

# Declare bodies
bodies=("shell" "core" "short_hub" "long_hub")

# Run the export python script
./export_stl.py ${model} ${bodies[@]} --output-dir build ${dry_run}