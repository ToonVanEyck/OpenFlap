#!/bin/bash

# Parse arguments
dry_run=""
if [[ "$1" == "--dry-run" ]]; then
    dry_run="--dry-run"
fi

# Check if the script is being run in the correct container or in GitHub Actions.
if [ "$(hostname)" != "ecad-mcad" ] && [ -z "$GITHUB_ACTIONS" ]; then
    echo -e "\e[31mError: This script is meant to be ran in the ecad-mcad container.\e[0m"
    exit 1
fi

BASE_DIR=$(git rev-parse --show-toplevel)
SOURCE_DIR=${BASE_DIR}/mechanical
BUILD_DIR=${BASE_DIR}/build/mechanical

# Declare FreeCAD model
model="$SOURCE_DIR/openflap_cad_model.FCStd"

# Declare bodies
bodies=("shell" "core" "short_hub" "long_hub" "flap_assembly_tool" "flap_assembly_guide" "french_cleat_mount")

# Run the export python script
$SOURCE_DIR/export_stl.py ${model} ${bodies[@]} --output-dir $BUILD_DIR ${dry_run}