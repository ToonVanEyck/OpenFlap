#!/bin/bash

# Function to print usage
print_usage() {
    echo "Usage: $0 [--build-only] [--target] [--qemu] [--skip-app] [--help]"
    echo "  --build-only   Only build the tests but don't run them."
    echo "  --target       Run tests on target."
    echo "  --qemu         Run tests in QEMU."
    echo "  --skip-app     Skip building the application."
    echo "  --help         Display this help message."
}

# Parse arguments
build_only=false
target=false
qemu=false
skip_app=false
all_success=true

while [[ "$#" -gt 0 ]]; do
    case $1 in
        --build-only) build_only=true ;;
        --target) target=true ;;
        --qemu) qemu=true ;;
        --skip-app) skip_app=true ;;
        --help) print_usage; exit 0 ;;
        *) echo -e "\e[31mUnknown parameter passed: $1\e[0m"; print_usage; exit 1 ;;
    esac
    shift
done

# Check if the script is being ran in the correct container.
if [ "$(hostname)" != "esp-idf" ]; then
    echo -e "\e[31mError: This script is meant to be ran in the esp-idf container.\e[0m"
    exit 1
fi

BASE_DIR=$(git rev-parse --show-toplevel)
CONTROLLER_SOURCE_DIR=${BASE_DIR}/software/controller_v2
CONTROLLER_BUILD_DIR=${BASE_DIR}/build/controller

mkdir -p ${CONTROLLER_SOURCE_DIR}
rm -rf ${CONTROLLER_BUILD_DIR}/*

# Build the application
if ! $skip_app; then
    echo -e "\033[1;36mBuilding Application\033[0m"
    idf.py -B ${CONTROLLER_BUILD_DIR} build -C ${CONTROLLER_SOURCE_DIR}
    [[ $? -ne 0 ]] && all_success=false
fi

# Build the tests
if $target || $qemu; then
    echo -e "\033[96mBuilding Tests\033[0m"
    python ${CONTROLLER_SOURCE_DIR}/build_tests.py -s ${CONTROLLER_SOURCE_DIR} -b ${CONTROLLER_BUILD_DIR} -t esp32 -m qemu
    [[ $? -ne 0 ]] && all_success=false
fi

# Run the tests
if ! $build_only; then
    if $target; then
        echo -e "\033[96mRunning Tests on target\033[0m"
        pytest --target esp32 -m target ${CONTROLLER_SOURCE_DIR} --build-dir ${CONTROLLER_BUILD_DIR} --embedded-services esp,idf
        [[ $? -ne 0 ]] && all_success=false
    fi
    if $qemu; then
        echo -e "\033[96mRunning Tests in QEMU\033[0m"
        pytest --target esp32 -m qemu ${CONTROLLER_SOURCE_DIR} --build-dir ${CONTROLLER_BUILD_DIR} --embedded-services qemu,idf --qemu-extra-args '-global driver=timer.esp32.timg,property=wdt_disable,value=true'
        [[ $? -ne 0 ]] && all_success=false
    fi
fi

# Return success only if there were no failures
if $all_success; then
    exit 0
else
    exit 1
fi

