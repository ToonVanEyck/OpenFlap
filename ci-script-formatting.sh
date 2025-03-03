#!/bin/bash

# This script uses clang-format to check the formatting of the source code files
# and codespell to check for common spelling mistakes in the source code files.
# The script can be run with the --dry-run argument to see the changes that would be made.
# This is script is used by the CI pipeline to check the formatting of the source code files.
# .cspell_ignore contains a list of words that may be ignored when looking for spelling mistakes.

# Check for --dry-run argument
CF_ARG="-i"
CS_ARG="-w"
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --dry-run)
            CF_ARG="-dry-run -Werror"
            CS_ARG=""
            ;;
        *)
            echo "Invalid option: $1" >&2
            echo "Usage: $0 [--dry-run]"
            exit 1
            ;;
    esac
    shift
done

# Check if the script is being run in the correct container or in GitHub Actions.
if [ "$(hostname)" != "esp-idf" ] && [ -z "$GITHUB_ACTIONS" ]; then
    echo -e "\e[31mError: This script is meant to be ran in the esp-idf container.\e[0m"
    exit 1
fi

BASE_DIR=$(git rev-parse --show-toplevel)

# List of ignored patterns
ignored_patterns=(
    'build'
    'software/module/lib/puya_libs'
    'software/controller_old'
    'software/controller/managed_components'
    'software/controller/components/u8g2*'
    'software/controller/components/networking/include/networking_default_config.h'
)

# Construct the find command with ignored patterns
find_command="find ${BASE_DIR}"
for pattern in "${ignored_patterns[@]}"; do
    find_command+=" -path '${BASE_DIR}/$pattern' -prune -o"
done

# For clang format, we only want to find .c and .h files
cf_find_command=${find_command}" \( -regex '.*\.\(c\|h\)' \) -print"
# For codespell, we want to find .c, .h, .md, .txt, .py, and .sh files
cs_find_command=${find_command}" \( -regex '.*\.\(c\|h\|md\|txt\|py\|sh\)' \) -print"

# Execute the constructed find command
eval $cf_find_command | xargs clang-format -style=file ${CF_ARG}
eval $cs_find_command | xargs codespell -I ${BASE_DIR}/.codespell_ignore ${CS_ARG}