#!/bin/sh
#
# The aim of this script is to clean the project's build configuration and variant
# specified on the command line.
#
# The essential portions of this script are marked with "# NEEDED":
# they may be adapted of course.

# Stop on first error.
set -e

# NEEDED: set the variable for the directory of this script.
HERE=$(dirname "$0")

printf "build.sh--> HERE=%s\n" $HERE

# Configure the project
cmake -DCMAKE_TOOLCHAIN_FILE="${HERE}/arm-none-eabi-toolchain.cmake" -GNinja -B${HERE}/build/$1 -DCMAKE_BUILD_TYPE=$1 ${HERE}

cd "${HERE}/build/$1"

# Build the project with ninja. 
ninja -v