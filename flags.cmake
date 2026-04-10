message("flasg.cmake files is processeing...")

if(USE_MCXA142)
    set(CMAKE_C_FLAGS "-mthumb -mcpu=cortex-m33+nodsp -specs=nano.specs")
    set(CMAKE_ASM_FLAGS "-mthumb -mcpu=cortex-m33+nodsp -specs=nano.specs")
    set(HAL_DEFINE "-DCPU_MCXA142VFM -DCPU_MCXA142VFM_cm33_nodsp")
endif()

# Rimuovo i flag di default di cmake -g oppure -O3 -DNDEBUG
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_ASM_FLAGS_DEBUG "")
set(CMAKE_ASM_FLAGS_RELEASE "")

# Compiler options
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE      
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    -fstack-usage
    $<$<COMPILE_LANGUAGE:C>: -fdata-sections -ffunction-sections>
    $<$<COMPILE_LANGUAGE:CXX>:

    # -Wno-volatile
    # -Wold-style-cast
    # -Wuseless-cast
    # -Wsuggest-override
    >
    $<$<COMPILE_LANGUAGE:ASM>: -c -x assembler-with-cpp -D__NEWLIB__>
    $<$<CONFIG:Debug>: -O0 -g3 -ggdb ${HAL_DEFINE} -DDEBUG=1 -D__USE_CMSIS -D__ATOLLIC__ -D__NEWLIB__ -D__STARTUP_CLEAR_BSS -ffreestanding -fno-common -fno-builtin>
    $<$<CONFIG:Release>: -Os -g0 -fno-strict-aliasing ${HAL_DEFINE} -DNDEBUG -D__USE_CMSIS -D__ATOLLIC__ -D__NEWLIB__ -D__STARTUP_CLEAR_BSS -ffreestanding -fno-common -fno-builtin>
    $<$<CONFIG:Spy>: -O0 -g3 -ggdb ${HAL_DEFINE} -DDEBUG=1 -DQ_SPY -D__USE_CMSIS -D__ATOLLIC__ -D__NEWLIB__ -D__STARTUP_CLEAR_BSS -ffreestanding -fno-common -fno-builtin>
)

# Linker options
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    -Xlinker -no-warn-rwx-segments
    -T${ProjDirPath}/app/src/linker_flash.ld  
    --specs=nosys.specs
    -flto    
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,--sort-section=alignment
    -Wl,--cref    
)