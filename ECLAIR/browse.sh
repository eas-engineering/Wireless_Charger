#!/bin/sh
# This script usually does not require adaptation.
#
# The aim of this file is to open the default browser on ECLAIR detailed
# reports and open the default file manager on the ECLAIR output directory.
#
# The essential portions of this script are marked with "# NEEDED":
# they may be adapted of course.


# Stop on first error.
set -e

# NEEDED: set the variable for the directory of this script.
HERE=$(dirname "$0")

# NEEDED: set the variable for the top directory for the project.
TOP=$(dirname "${HERE}")

# Print usage information for the script and exit.
usage() {
    echo "Usage: browse.sh RULESET ARGS..." 1>&2
    echo "  where ARGS... are the arguments for prepare.sh, clean.sh and build.sh" 1>&2
    exit 2
}

# <RULESET> is passed as first argument.
RULESET=$1

# Print usage information and exit if RULESET is missing.
if [ -z "${RULESET}" ]; then
    usage
fi

# Remove first argument.
shift

# <BUILD_TYPE> is passed as second argument.
export BUILD_TYPE=$1

# shellcheck source=./eclair_settings.sh
# Load script settings for the sub-project specified on the command line.
. "${HERE}/eclair_settings.sh"

# Check that the ECLAIR output directory exists: give error and
# exit otherwise.
if [ ! -d "${ECLAIR_OUTPUT_DIR}" ]; then
    echo "Directory ${ECLAIR_OUTPUT_DIR} doesn't exists"
    exit 1
fi

# Set the variable for the ECLAIR binary output directory.
ECLAIR_DATA_DIR=${ECLAIR_OUTPUT_DIR}/.data
# Set the variable for the ECLAIR project database file.
PROJECT_ECD=${ECLAIR_DATA_DIR}/PROJECT.ecd

# Open the file manager on the ECLAIR output directory.
if type xdg-open >/dev/null 2>&1
then
  xdg-open "${ECLAIR_OUTPUT_DIR}"
elif type open >/dev/null 2>&1
then
  open "${ECLAIR_OUTPUT_DIR}"
else
  echo "See your analysis output in \"${ECLAIR_OUTPUT_DIR}\"."
fi

# Restart the eclair_report server and open the default browser on
# ECLAIR detailed reports.
"${ECLAIR_PATH}eclair_report" "-db='${PROJECT_ECD}'" "-eval_file='${HERE}/browse.ecl'"
