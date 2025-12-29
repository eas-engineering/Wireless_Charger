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

printf "clean.sh--> HERE=%s\n" $HERE

# If the output folder exist clean the project
if [ -d "${HERE}/build/$1" ]; then
   cd "${HERE}/build/$1"
   ninja -v -t clean
fi