#!/bin/bash

# Define the line to be added
line_to_add='[ -f $HOME/.bashrc_devcontainer_addons ] && . $HOME/.bashrc_devcontainer_addons'

# Check if .bashrc exists and is writable
if [ ! -f "$HOME/.bashrc" ] || [ ! -w "$HOME/.bashrc" ]; then
    exit 0
fi
# Check if the line is already in .bashrc
if ! grep -Fxq "$line_to_add" "$HOME/.bashrc"; then
    # If not, add the line to .bashrc
    echo "" >> "$HOME/.bashrc"
    echo "# Load bashrc devcontainer addons" >> "$HOME/.bashrc"
    echo "$line_to_add" >> "$HOME/.bashrc"
fi