#!/bin/bash

# Parse options
generate_gerber=false
generate_stl=false
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --pcb)
            generate_gerber=true
            ;;
        --3d)
            generate_stl=true
            ;;
        *)
            echo "Invalid option: $1" >&2
            echo "Usage: $0 [--pcb] [--3d]"
            exit 1
            ;;
    esac
    shift
done

# Check if the script is being ran in the correct container.
if [ "$(hostname)" != "ecad-mcad" ]; then
    echo -e "\e[31mError: This script is meant to be ran in the ecad-mcad container.\e[0m"
    exit 1
fi

BASE_DIR=$(git rev-parse --show-toplevel)
SOURCE_DIR=${BASE_DIR}/hardware/module/flaps
BUILD_DIR=${BASE_DIR}/build/flaps
BUILD_DIR_3D=${BUILD_DIR}/3d
BUILD_DIR_PCB=${BUILD_DIR}/pcb

# Try generating the flaps
mkdir -p ${BUILD_DIR_3D}
mkdir -p ${BUILD_DIR_PCB}
rm -rf ${BUILD_DIR_3D}/*
rm -rf ${BUILD_DIR_PCB}/*
echo -e "\033[1;36mGenerating Flaps\033[0m"
echo -e "\033[96mGenerate .zip with browser based flap generator\033[0m"
python3 ${SOURCE_DIR}/browser_based_flap_generator/test/browser_based_flap_generator_test.py ${SOURCE_DIR}/browser_based_flap_generator/index.html ${BUILD_DIR}

if [ "$generate_gerber" = true ]; then
    echo -e "\033[96mConvert .zip with gerbolyze\033[0m"
    ${SOURCE_DIR}/flaps_to_pcb.sh ${BUILD_DIR}/flaps.zip ${BUILD_DIR_PCB}
fi

if [ "$generate_stl" = true ]; then
    echo -e "\033[96mConvert .zip with FreeCAD\033[0m"
    ${SOURCE_DIR}/flaps_to_3d.py ${BUILD_DIR}/flaps.zip ${BUILD_DIR_3D}
fi