#! /usr/bin/env python

import os
import sys
import subprocess
import glob
import argparse
import re
import shutil
import esptool
import time
from colorama import Fore, Style

# from pathlib import Path

if os.path.join(os.environ["IDF_PATH"], "tools", "ci") not in sys.path:
    sys.path.append(os.path.join(os.environ["IDF_PATH"], "tools", "ci"))

if (
    os.path.join(os.environ["IDF_PATH"], "tools", "ci", "python_packages")
    not in sys.path
):
    sys.path.append(
        os.path.join(os.environ["IDF_PATH"], "tools", "ci", "python_packages")
    )

if (
    os.path.join(os.environ["IDF_PATH"], "components", "partition_table")
    not in sys.path
):
    sys.path.append(
        os.path.join(os.environ["IDF_PATH"], "components", "partition_table")
    )

from idf_pytest.script import get_pytest_cases
from idf_build_apps import CMakeApp
from gen_esp32part import PartitionTable
import shlex
import json


from idf_build_apps.constants import (
    BuildStatus,
)

# Constants
IDF_PATH = os.environ.get("IDF_PATH", "/opt/esp/idf")
UNITY_APP_PATH = os.path.join(IDF_PATH, "tools/unit-test-app/")

if not os.path.exists(UNITY_APP_PATH):
    print(f"{Fore.RED}Unity app path does not exist: {UNITY_APP_PATH}{Style.RESET_ALL}")
    exit(1)

# Variables
build_dir = os.getcwd()
source_dir = os.getcwd()
active_test_build_dir = None
pytest_files = []
components_path = []
testable_components_paths = []
testable_components = []
component = None
tty_device = None
project_name = None

qemu_process = None
openocd_process = None


# Check if .vscode/settings.json exists and set tty_device if idf.port is set
vscode_settings_path = os.path.join(os.getcwd(), ".vscode", "settings.json")
if os.path.exists(vscode_settings_path):
    try:
        with open(vscode_settings_path, "r") as settings_file:
            settings = json.load(settings_file)
            if "idf.port" in settings:
                tty_device = settings["idf.port"]
                print(f"Using tty_device from .vscode/settings.json: {tty_device}")
    except json.JSONDecodeError:
        pass

#######################################################################################################
####                                         UTIL FUNCTIONS                                        ####
#######################################################################################################


def set_directories(source_dir_arg, build_dir_arg):
    global source_dir
    global build_dir
    global active_test_build_dir
    if source_dir_arg:
        source_dir = os.path.abspath(source_dir_arg)
        build_dir = source_dir

    if build_dir_arg:
        build_dir = os.path.abspath(build_dir_arg)
        active_test_build_dir = os.path.join(build_dir, "component")


def set_component_list():
    global pytest_files
    global components_path
    global testable_components_paths
    global testable_components
    pytest_files = glob.glob(
        os.path.join(source_dir, "**/test", "pytest_*.py"), recursive=True
    )
    components_path = os.path.join(source_dir, "components")
    testable_components_paths = [
        os.path.dirname(os.path.dirname(pytest_files)) for pytest_files in pytest_files
    ]
    testable_components = [
        os.path.basename(component_path) for component_path in testable_components_paths
    ]


def get_project_name():
    global project_name
    cmake_file = os.path.join(source_dir, "CMakeLists.txt")
    try:
        with open(cmake_file, "r") as file:
            content = file.read()
    except FileNotFoundError:
        print("CMakeLists.txt not found!")
        exit(1)

    # Extract the project name
    match = re.search(r"project\(([^ ]+)\)", content, re.IGNORECASE)
    if match:
        project_name = match.group(1)
        print(f"Project name: {project_name}")
    else:
        print("Project name not found in CMakeLists.txt!")
        exit(1)


def get_component(name, path):
    global component
    component = name
    if path:
        match = re.search(r"components/([^/]+)", path)
        if match:
            component = match.group(1)
        if component is None:
            print(
                f"{Fore.RED}Invalid Component Path: '{path}' does not belong to a component.{Style.RESET_ALL}"
            )


def component_test_has_mark(component, mark):
    cases = get_pytest_cases(os.path.join(components_path, component))
    for case in cases:
        for app in case.apps:
            if app.config == mark:
                return True
    return False


#######################################################################################################
####                                        FLASH FUNCTIONS                                        ####
#######################################################################################################


def flash_binary(component):
    global tty_device

    mark = "target"  # we can only flash real hardware.

    # Configure the build path
    if component is None:
        active_build_path = build_dir
    else:
        active_build_path = os.path.join(build_dir, component, f"build_esp32_{mark}")

    # Validate the flash arguments
    abs_flash_args = os.path.join(active_build_path, "abs_flash_args")
    assert os.path.exists(
        abs_flash_args
    ), f"{Fore.RED}Flash arguments not found: {abs_flash_args}{Style.RESET_ALL}"

    # Flash the binary
    esptool_arguments = (
        f"esptool.py "
        f"--chip esp32 "
        f"-b 460800 "
        f"--before default_reset "
        f"--after hard_reset "
        f"write_flash "
        f"@{abs_flash_args} "
    )
    try:
        flash_process = subprocess.Popen(
            shlex.split(esptool_arguments),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )

        # Try to get the tty device
        for line in flash_process.stdout:
            print(line, end="")  # Print the output line by line
            sys.stdout.flush()
            match = re.search(r"/dev/tty.*[0-9]+", line)
            if match:
                tty_device = match.group(0).split()[0]

        # Wait for the process to finish
        flash_process.wait()

    except Exception as e:
        assert (
            False
        ), f"{Fore.RED}Failed to flash binary on target: {e}{Style.RESET_ALL}"


#######################################################################################################
####                                        BUILD FUNCTIONS                                        ####
#######################################################################################################


def build_binary(component, mark):
    if component is None:
        # App configuration
        print(f"{Fore.CYAN}Configuring application binary for {mark}.{Style.RESET_ALL}")
        active_source_dir = source_dir
        active_build_dir = build_dir
        sdkconfig_path = os.path.join(active_source_dir, f"sdkconfig.{mark}")
        partition_table_path = None
        cmake_vars = {
            "QEMU": "1" if mark == "qemu" else "0",
        }
        component = project_name
        elf_file = os.path.join(active_build_dir, f"{project_name}.elf")

    else:
        # Component configuration
        print(
            f"{Fore.CYAN}Configuring component test binary for {mark}.{Style.RESET_ALL}"
        )
        active_source_dir = UNITY_APP_PATH
        active_build_dir = os.path.join(build_dir, component, f"build_esp32_{mark}")
        test_path = os.path.join(components_path, component, "test")
        sdkconfig_path = os.path.join(test_path, f"sdkconfig.{mark}")
        partition_table_path = os.path.join(test_path, "test_partition_table.csv")
        cmake_vars = {
            "EXTRA_COMPONENT_DIRS": components_path,
            "TEST_COMPONENTS": component,
            "QEMU": "1" if mark == "qemu" else "0",
        }
        elf_file = os.path.join(active_build_dir, "unit-test-app.elf")

    if mark == "qemu":
        merge_fill_arg = "--fill-flash-size 16MB"
    else:
        merge_fill_arg = ""

    # Validate paths
    assert os.path.exists(
        active_source_dir
    ), f"{Fore.RED}Source directory not found: {active_source_dir}{Style.RESET_ALL}"
    assert os.path.exists(
        sdkconfig_path
    ), f"{Fore.RED}SDKConfig not found: {sdkconfig_path}{Style.RESET_ALL}"
    assert partition_table_path is None or os.path.exists(
        partition_table_path
    ), f"{Fore.RED}Partition table not found: {partition_table_path}{Style.RESET_ALL}"

    # Build the component
    print(f"{Fore.CYAN}Building binary.{Style.RESET_ALL}")
    idf_app = CMakeApp(
        app_dir=active_source_dir,
        target="esp32",
        config_name=mark,
        sdkconfig_defaults_str=sdkconfig_path,
        build_dir=active_build_dir,
        depends_components=[testable_components_paths],
        cmake_vars=cmake_vars,
    )
    idf_app.build()

    # Validate build status
    assert (
        idf_app.build_status == BuildStatus.SUCCESS
    ), f"{Fore.RED}Failed to build {component} binary{Style.RESET_ALL}"

    # Compile the partition table
    if partition_table_path:
        with open(partition_table_path, "rb") as partitionTable:
            table, input_is_binary = PartitionTable.from_file(partitionTable)
            table.verify()
            output = table.to_binary()
            partition_table_dir = os.path.join(active_build_dir, "partition_table")
            partition_table_bin = os.path.join(
                partition_table_dir, "partition-table.bin"
            )
            os.makedirs(partition_table_dir, exist_ok=True)
            with open(
                partition_table_bin,
                "wb",
            ) as f:
                f.write(output)

    # Create absolute pth flash args
    print(f"{Fore.CYAN}Configuring flash arguments.{Style.RESET_ALL}")
    flash_args = os.path.join(active_build_dir, "flash_args")
    abs_flash_args = os.path.join(active_build_dir, "abs_flash_args")

    with open(flash_args, "r") as infile, open(abs_flash_args, "w") as outfile:
        for line in infile:
            parts = line.split()
            if parts[-1].endswith(".bin"):
                parts[-1] = os.path.join(active_build_dir, parts[-1])
            line = " ".join(parts) + "\n"
            outfile.write(line)

    # Merge binaries
    print(f"{Fore.CYAN}Merging binaries.{Style.RESET_ALL}")
    merged_bin = os.path.join(active_build_dir, "merged_binary.bin")
    try:
        esptool_arguments = (
            f"--chip esp32 "
            f"merge_bin "
            f"{merge_fill_arg} "
            f"-o {merged_bin} "
            f"@{abs_flash_args} "
        )
        esptool.main(esptool_arguments.split())
    except Exception as e:
        assert False, f"{Fore.RED}Failed to merge binary: {e}{Style.RESET_ALL}"

    # Copy to directory
    print(f"{Fore.CYAN}Copying to debug directory.{Style.RESET_ALL}")
    debug_dir = os.path.join(build_dir, "debug")
    os.makedirs(debug_dir, exist_ok=True)
    try:
        shutil.copy(merged_bin, os.path.join(debug_dir, "debug.bin"))
        shutil.copy(elf_file, os.path.join(debug_dir, "debug.elf"))
    except shutil.SameFileError:
        pass


#######################################################################################################
####                                        MONITOR FUNCTIONS                                      ####
#######################################################################################################


def monitor_launch(mark):
    elf_file = os.path.join(build_dir, "debug", f"debug.elf")
    port_arg = ""
    if mark == "qemu":
        port_arg = "-p socket://localhost:5555"
    elif tty_device:
        port_arg = f"-p {tty_device}"
    monitor_command = (
        f"python {IDF_PATH}/tools/idf_monitor.py "
        f"{port_arg} "
        f"-b 115200 "
        f"--toolchain-prefix xtensa-esp32-elf- "
        f"--target esp32 "
        f"--revision 0 "
        f"--no-reset "
        f"{elf_file}"
    )
    time.sleep(1)
    subprocess.run(shlex.split(monitor_command))


#######################################################################################################
####                                         pytest FUNCTIONS                                      ####
#######################################################################################################


def pytest(component, mark):
    build_path = os.path.join(build_dir, component)
    source_path = os.path.join(components_path, component)

    qemu_extra_args = ""
    services = ""
    if "qemu" in mark:
        qemu_extra_args = (
            f'--qemu-extra-args "'
            f"-global driver=timer.esp32.timg,property=wdt_disable,value=true "
            f"-nic user,model=open_eth,hostfwd=tcp::80-:80 "
            f'"'
        )
        services += "qemu,"

    if "target" in mark:
        services += "esp,"

    mark_str = ",".join(mark)

    assert os.path.exists(
        build_path
    ), f"{Fore.RED}Build directory not found: {build_path}{Style.RESET_ALL}"
    assert os.path.exists(
        source_path
    ), f"{Fore.RED}Source directory not found: {source_path}{Style.RESET_ALL}"

    pytest_command = (
        f"pytest "
        f"{source_path} "
        f"--build-dir {build_path} "
        f"-m {mark_str} "
        f"--embedded-services {services}idf "
        f"{qemu_extra_args} "
        f"--port /dev/ttyUSB1 "
        f"--baud 115200 "
    )
    print(pytest_command)
    subprocess.run(shlex.split(pytest_command))


#######################################################################################################
####                                              MAIN                                             ####
#######################################################################################################

if __name__ == "__main__":
    # Set up argument parser
    parser = argparse.ArgumentParser(
        description="Build, Launch and Test utility script."
    )

    parser.add_argument(
        "action",
        choices=["build", "monitor", "run", "flash", "pytest"],
        help="Action to perform: build, monitor, run, flash or pytest.",
    )

    parser.add_argument(
        "-b",
        "--build-dir",
        required=False,
        help="Project build directory.",
    )
    parser.add_argument(
        "-s",
        "--source-dir",
        required=False,
        help="Project source directory.",
    )

    mark_group = parser.add_mutually_exclusive_group(required=False)
    mark_group.add_argument(
        "--qemu",
        required=False,
        action="store_true",
        help="Build/Launch/Test the project or component on QEMU.",
    )
    mark_group.add_argument(
        "--target",
        required=False,
        action="store_true",
        help="Build/Launch/Test the project or component on target hardware.",
    )

    component_group = parser.add_mutually_exclusive_group(required=False)

    component_group.add_argument(
        "-c",
        "--component",
        type=str,
        required=False,
        help="Build the test binary of the component instead of the whole project.",
    )

    component_group.add_argument(
        "--component-from-path",
        type=str,
        required=False,
        help="Build the test binary for the component which contains the provided file.",
    )

    parser.add_argument(
        "-l",
        "--list-testable-components",
        required=False,
        action="store_true",
        help="List all the components available for testing.",
    )

    parser.add_argument(
        "--rebuild",
        required=False,
        action="store_true",
        help="Rebuild the project or component.",
    )

    # Parse the arguments
    args = parser.parse_args()

    # Configure the build and source directories
    set_directories(args.source_dir, args.build_dir)

    # Get list of the components
    set_component_list()

    # Get the project name
    get_project_name()

    # List the testable components
    if args.list_testable_components:
        print(f"{Fore.CYAN}Listing testable Components: {Style.RESET_ALL}")
        for component in testable_components:
            print(component)
        exit(0)

    # Get the component to build
    if args.component or args.component_from_path:
        get_component(args.component, args.component_from_path)
        # Check if the component is testable
        if component not in testable_components:
            print(
                f"{Fore.RED}Invalid Component: '{component}' is not a testable component.{Style.RESET_ALL}"
            )
            print(f"{Fore.CYAN}Listing testable Components: {Style.RESET_ALL}")
            for component in testable_components:
                print(component)
            exit(1)

    status_ok = True

    mark = "qemu" if args.qemu else "target"

    # Build Command
    if args.action == "build" or args.rebuild:
        print(
            f"{Fore.CYAN}{Style.BRIGHT}Building {component if component else project_name} binary for {mark}.{Style.RESET_ALL}"
        )
        build_binary(component, mark)

    # Monitor Command
    if args.action == "monitor":
        print(f"{Fore.CYAN}{Style.BRIGHT}Starting monitor.{Style.RESET_ALL}")
        monitor_launch(mark)

    # Flash Command
    if args.action == "flash":
        assert (
            not args.qemu
        ), f"{Fore.RED}Flash is not applicable to QEMU.{Style.RESET_ALL}"
        print(
            f"{Fore.CYAN}{Style.BRIGHT}Flashing {component if component else project_name} binary.{Style.RESET_ALL}"
        )
        flash_binary(component)

    # Run Command: (Flash & Monitor)
    if args.action == "run":
        assert (
            not args.qemu
        ), f"{Fore.RED}Use the debugger to run on QEMU.{Style.RESET_ALL}"
        flash_binary(component)
        monitor_launch("target")

    if args.action == "pytest":
        if component:
            print(
                f"{Fore.CYAN}{Style.BRIGHT}Running pytest for {component}.{Style.RESET_ALL}"
            )
            mark = []
            if args.qemu:
                mark.append("qemu")
            if args.target:
                mark.append("target")
            pytest(component, mark)
        else:
            print(
                f"{Fore.RED}Running pytest is only applicable to components.{Style.RESET_ALL}"
            )
