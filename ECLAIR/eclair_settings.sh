#!/bin/sh
# This script must not be renamed: it is referenced by analyze.sh and
# browse.sh.
#
# The aim of this script is to set the variables required by the by
# the scripts mentioned above for the sub-project specified on the
# command line.
#
# The essential portions of this script are marked with "# NEEDED":
# they may be adapted of course.

# NEEDED: set the directory where the ECLAIR installation's "bin" directory are located.
# If empty, the binaries are searched using the PATH environment variable;
# otherwise the specified directory shall end with / (slash).
#export ECLAIR_PATH=

# NEEDED: set the variable for the ECLAIR analysis output directory.
export ECLAIR_OUTPUT_DIR=${HERE}/out_${BUILD_TYPE}_${RULESET}

# NEEDED: set the evironment variable used by ECLAIR to intercepted
# the selected toolchain components: absolute file paths are used here
# (this is always the safest choice).
export CC_ALIASES="'/opt/st/stm32cubeclt/GNU-tools-for-STM32/bin/arm-none-eabi-gcc'"
export CXX_ALIASES="'/opt/st/stm32cubeclt/GNU-tools-for-STM32/bin/arm-none-eabi-g++'"
export AS_ALIASES="'/opt/st/stm32cubeclt/GNU-tools-for-STM32/bin/arm-none-eabi-as'"
export AR_ALIASES="'/opt/st/stm32cubeclt/GNU-tools-for-STM32/bin/arm-none-eabi-ar'"
export LD_ALIASES="'/opt/st/stm32cubeclt/GNU-tools-for-STM32/bin/arm-none-eabi-ld'"
