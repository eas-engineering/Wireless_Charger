set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

SET (CMAKE_EXECUTABLE_SUFFIX ".elf")

message("arm-none-eabi-toochain.cmake files is processing...")

SET(TOOLCHAIN_DIR /usr/local/mcuxpressoide-25.6.136/ide/plugins/com.nxp.mcuxpresso.tools.linux_25.6.0.202501151204/tools/bin)

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Imposta il compilatore ARM
set(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_DIR}/arm-none-eabi-gcc)
set(CMAKE_OBJCOPY ${TOOLCHAIN_DIR}/arm-none-eabi-objcopy)
set(CMAKE_SIZE ${TOOLCHAIN_DIR}/arm-none-eabi-size)
set(CMAKE_OBJDUMP ${TOOLCHAIN_DIR}/arm-none-eabi-objdump)

set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

# Specifica il percorso delle librerie e degli include
SET(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_DIR}/arm-none-eabi)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_EXECUTABLE_SUFFIX_ASM     ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C       ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX     ".elf")