#!/bin/bash
# This script usually does not require adaptation.
#
# The aim of this script is to perform a build and corresponding
# analysis for the sub-project specified on the command line.
#
# The essential portions of this script are marked with "# NEEDED":
# they may be adapted of course.

# Stop on first error.
set -e
set -o pipefail

# NEEDED: set the variable for the absolute directory of this script.
HERE=$(
    cd "$(dirname "$0")"
    echo "${PWD}"
)

# NEEDED: set the variable for the top absolute directory for the project.
TOP=$(dirname "${HERE}")

# Print usage information for the script and exit.
usage() {
    echo "Usage: analyze.sh RULESET ARGS..." 1>&2
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

# Set the variable for analysis ECL file specific to <RULESET>.
ANALYSIS_ECL=${HERE}/analysis_${RULESET}.ecl

# Set the variable for report ECL file specific to <RULESET>.
REPORT_ECL=${HERE}/report_${RULESET}.ecl
# Use default if a specific report ECL file does not exist: report__cli.ecl.
if [ ! -f "${REPORT_ECL}" ]
then
    REPORT_ECL=${HERE}/report__cli.ecl
fi

# NEEDED: set the variable for the project root directory.
export ECLAIR_PROJECT_ROOT=${TOP}

# <BUILD_TYPE> is passed as second argument.
export BUILD_TYPE=$1

# shellcheck source=./eclair_settings.sh
# Load script settings for the sub-project specified on the command line.
. "${HERE}/eclair_settings.sh"

# Check that the analysis configuration file exists: give error and
# exit otherwise.
if [ ! -f "${ANALYSIS_ECL}" ]; then
    echo "File ${ANALYSIS_ECL} does not exist"
    exit 2
fi

# NEEDED: set the variable for the ECLAIR binary output directory.
export ECLAIR_DATA_DIR=${ECLAIR_OUTPUT_DIR}/.data
# NEEDED: set the variable for the ECLAIR analysis log absolute file path.
export ECLAIR_DIAGNOSTICS_OUTPUT=${ECLAIR_OUTPUT_DIR}/ANALYSIS.log
# Set the variable for the ECLAIR project database file.
PROJECT_ECD=${ECLAIR_DATA_DIR}/PROJECT.ecd
# Set the variable for the clean log file.
CLEAN_LOG=${ECLAIR_OUTPUT_DIR}/CLEAN.log
# Set the variable for the build log file.
BUILD_LOG=${ECLAIR_OUTPUT_DIR}/BUILD.log
# Set the variable for the ECLAIR report log file.
REPORT_LOG=${ECLAIR_OUTPUT_DIR}/REPORT.log

# Remove the old ECLAIR output directory, if any, and (re-) create it.
rm -rf "${ECLAIR_OUTPUT_DIR}"
mkdir -p "${ECLAIR_DATA_DIR}"

# Clean the sub-project.
"${TOP}/clean.sh" "$@" 2>&1 | tee "${CLEAN_LOG}"

# Build the sub-project in an ECLAIR environment with the given configuration.
"${ECLAIR_PATH}eclair_env" "-eval_file='${ANALYSIS_ECL}'" -- "${TOP}/build.sh" "$@" 2>&1 | tee "${BUILD_LOG}"

# Generate the project database.
"${ECLAIR_PATH}eclair_report" "-create_db='${PROJECT_ECD}'" "${ECLAIR_DATA_DIR}"/FRAME.*.ecb \
    -load "-eval_file='${REPORT_ECL}'" 2>&1 | tee "${REPORT_LOG}"

if [ -t 1 ]
then
    # Browse the obtained results.
    "${HERE}/browse.sh" "${RULESET}" "$@"
fi
