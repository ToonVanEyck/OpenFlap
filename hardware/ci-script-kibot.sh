#!/bin/bash

# Parse options
check_modified=false
dry_run=false
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --modified)
            check_modified=true
            ;;
        --dry-run)
            dry_run=true
            ;;
        *)
            echo "Invalid option: $1" >&2
            exit 1
            ;;
    esac
    shift
done

# Function to check if any file in a module directory has changed
has_changed() {
    local dir=$1
    if git status --porcelain | grep -q "$dir"; then
        return 0
    else
        return 1
    fi
}

# Set variables based on the argument
if $dry_run; then
    message="Validating"
    kibot_preflight_only="-i"
else
    message="Generating"
    kibot_preflight_only=""
fi

# Check if the script is being ran in the correct container.
if [ "$(hostname)" != "ecad-mcad" ]; then
    echo -e "\e[31mError: This script is meant to be ran in the ecad-mcad container.\e[0m"
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

# Remove unchanged modules if --modified option is set
if $check_modified; then
    for name in "${!modules[@]}"; do
        path=${modules[$name]}
        if ! has_changed "$path"; then
            unset "modules[$name]"
        fi
    done
fi

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