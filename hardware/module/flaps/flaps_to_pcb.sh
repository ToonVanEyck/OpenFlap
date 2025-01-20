#!/bin/bash

# Function to display help message
show_help() {
    echo "Usage: $0 input_zip output_dir"
    echo
    echo "Arguments:"
    echo "  input_zip    Path to the input .zip file containing flap SVG files."
    echo "  output_dir   Directory where the output flap gerber files will be saved."
    echo
    echo "Options:"
    echo "  -h, --help   Show this help message and exit"
}

# Check for --help or -h option
if [[ "$1" == "--help" || "$1" == "-h" ]]; then
    show_help
    exit 0
fi

# Check for the correct number of arguments
if [[ $# -ne 2 ]]; then
    echo "Error: Incorrect number of arguments"
    show_help
    exit 1
fi

input_zip="$1"
output_dir="$2"

# Unzip the input file
unzip -o "$input_zip" -d /tmp/flaps

# Loop through each .svg file in the temp directory
for svg_file in /tmp/flaps/*.svg; do
    # Get the filename without extension
    filename=$(basename "$svg_file" .svg)
    
    # Call gerbolyze convert command
    echo -e "\033[94mGerbolyze $svg_file\033[0m"
    gerbolyze convert -n altium "$svg_file" "$output_dir/$filename.zip"
done

# Remove the temporary directory
rm -rf /tmp/flaps