# Versione minima di CMake richiesta
cmake_minimum_required(VERSION 3.18)

# CURRENT DIRECTORY
SET(ProjDirPath ${CMAKE_CURRENT_SOURCE_DIR})

# Includo le configurazioni del progetto
include(${CMAKE_CURRENT_SOURCE_DIR}/config.cmake)

if(USE_QPC)

  if(NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libraries/middlewares/qpc)
    message("Clone repo QPC...")
    execute_process(
      COMMAND git submodule add --force https://github.com/eas-engineering/qpc.git libraries/middlewares/qpc  
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    execute_process(
      COMMAND git submodule update --init --recursive
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libraries/middlewares/qpc
    )
  endif()

  # Aggiungo i path della cartella CMSIS esterna all'SDK di QPC dove cercare i file
  list(APPEND CMAKE_MODULE_PATH        
      ${CMAKE_CURRENT_LIST_DIR}/libraries/middlewares/qpc/3rd_party/cmake
      ${CMAKE_CURRENT_LIST_DIR}/libraries/middlewares/qpc
  )

  # include general project config & import qpc
  set(QPC_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/libraries/middlewares/qpc)
  include(qpc_sdk_import)
endif()

# Nome del progetto
project(${PROJECT_NAME})

# Core project settings
enable_language(C CXX ASM)
message("Build type: " ${CMAKE_BUILD_TYPE})

# Aggiungi il tuo eseguibile/target
add_executable(${PROJECT_NAME})

# Includo i vari flags di compilazione
include(${CMAKE_CURRENT_SOURCE_DIR}/flags.cmake)

# Aggiungi le sottodirectory dei tuoi file sorgente
add_subdirectory(app/src)

include_directories(${PROJECT_BINARY_DIR})

if(NOT USE_MCXA142)
    message(FATAL_ERROR "You have to setup a microcontroller type! (USE_MCXAXXX)")
endif()

# Controllo e nel caso scarico la libreria CMSIS-CORE
if(USE_MCXA142)
  if(NOT IS_DIRECTORY ${PROJECT_SOURCE_DIR}/libraries/drivers/cmsis_core)
    message("Clone repo CMSIS-CORE...")
    execute_process(
      COMMAND git submodule add --force -b release/24.12.00 https://github.com/eas-engineering/mcu-sdk-cmsis.git libraries/drivers/cmsis_core  
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
  endif()
  set(HAL_DRIVER  "MCXA142.h")
endif()

# Creo il file hal_driver.h pre includere l'header file corretto in base
# al tipo di microcontrollore
configure_file(project_settings.conf ${CMAKE_CURRENT_SOURCE_DIR}/app/src/inc/project_settings.h)

# Include CMSIS Core
include_directories(${PROJECT_SOURCE_DIR}/libraries/drivers/cmsis_core/Core/Include)

# Controllo e nel caso scarico la libreria CMSIS-DEVICE in base al tipo di microcontrollore
if(USE_MCXA142)
  if(NOT IS_DIRECTORY ${PROJECT_SOURCE_DIR}/libraries/drivers/cmsis_device)
    message("Clone repo CMSIS-DEVICE...")
    execute_process(
      COMMAND git submodule add --force -b release/25.12.00 https://github.com/eas-engineering/mcux-devices-mcx.git libraries/drivers/cmsis_device
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
  endif()
  if (NOT EXISTS "${PROJECT_SOURCE_DIR}/app/src/startup_mcxa142.s")
    configure_file("${PROJECT_SOURCE_DIR}/libraries/drivers/cmsis_device/MCXA/MCXA142/gcc/startup_MCXA142.S" "${PROJECT_SOURCE_DIR}/app/src/startup_mcxa142.s" COPYONLY)
  endif()
  if (NOT EXISTS "${PROJECT_SOURCE_DIR}/app/src/system_mcxa142.c")
    configure_file("${PROJECT_SOURCE_DIR}/libraries/drivers/cmsis_device/MCXA/MCXA142/system_MCXA142.c" "${PROJECT_SOURCE_DIR}/app/src/system_mcxa142.c" COPYONLY)
  endif()
  if (NOT EXISTS "${PROJECT_SOURCE_DIR}/app/src/linker_flash.ld")
    configure_file("${PROJECT_SOURCE_DIR}/libraries/drivers/cmsis_device/MCXA/MCXA142/gcc/MCXA142_flash.ld" "${PROJECT_SOURCE_DIR}/app/src/linker_flash.ld" COPYONLY)
  endif()        
endif()

# Include CMSIS Device
if(USE_MCXA142)
  include_directories(${PROJECT_SOURCE_DIR}/libraries/drivers/cmsis_device/MCXA/MCXA142)
  include_directories(${PROJECT_SOURCE_DIR}/libraries/drivers/cmsis_device/MCXA/MCXA153/drivers)
  include_directories(${PROJECT_SOURCE_DIR}/libraries/drivers/cmsis_device/MCXA/periph)
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/cmsis_device/MCXA/MCXA153/drivers/fsl_clock.c)
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/cmsis_device/MCXA/MCXA153/drivers/fsl_edma_soc.c)
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/cmsis_device/MCXA/MCXA153/drivers/fsl_reset.c)
endif()

# Controllo e nel caso scarico la libreria MCUXpresso SDK CORE in base al tipo di microcontrollore
if(USE_MCXA142)
  if(NOT IS_DIRECTORY ${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core)
    message("Clone repo HAL...")
    execute_process(
      COMMAND git submodule add --force -b release/25.12.00 https://github.com/eas-engineering/mcuxsdk-core.git libraries/drivers/mcuxsdk_core
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
  endif()  
  include_directories(${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/common)
  include_directories(${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/port)
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/common/fsl_common.c)
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/common/fsl_common_arm.c)
  
  include_directories(${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/edma4)
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/edma4/fsl_edma.c)

  include_directories(${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/gpio)
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/gpio/fsl_gpio.c)

  include_directories(${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/lpi2c)
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/lpi2c/fsl_lpi2c.c)
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/lpi2c/fsl_lpi2c_edma.c)

  include_directories(${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/lpuart)
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/lpuart/fsl_lpuart.c) 
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/lpuart/fsl_lpuart_edma.c)  

  include_directories(${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/mcx_spc)
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/mcx_spc/fsl_spc.c)

  include_directories(${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/lpadc)
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/lpadc/fsl_lpadc.c)  

  include_directories(${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/pwm)
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/pwm/fsl_pwm.c)    

  include_directories(${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/ctimer)
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/ctimer/fsl_ctimer.c)     

  include_directories(${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/inputmux)
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/inputmux/fsl_inputmux.c)      
  
    include_directories(${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/wwdt)
  target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/libraries/drivers/mcuxsdk_core/drivers/wwdt/fsl_wwdt.c)       
endif()

# Include il layer BSP
add_subdirectory(libraries/drivers/BSP)

# Include il framework EDF per la gestione delle macchine a stati
if(USE_EDF_FRAMEWORK)
  if(NOT IS_DIRECTORY ${PROJECT_SOURCE_DIR}/libraries/middlewares/edf)
  message("Clone repo EDF...")
  execute_process(
    COMMAND git submodule add --force https://github.com/eas-engineering/edf.git libraries/middlewares/edf  
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
  endif()
  add_subdirectory(libraries/middlewares/edf)

  if (NOT EXISTS "${PROJECT_SOURCE_DIR}/app/src/inc/edf_cfg.h")
    configure_file("${PROJECT_SOURCE_DIR}/libraries/middlewares/edf/src/include/edf_cfg_template.h" "${PROJECT_SOURCE_DIR}/app/src/inc/edf_cfg.h" COPYONLY)
  endif()

  if (NOT EXISTS "${PROJECT_SOURCE_DIR}/app/src/inc/global_signals.h")
    configure_file("${PROJECT_SOURCE_DIR}/libraries/middlewares/edf/src/include/global_signal_template.h" "${PROJECT_SOURCE_DIR}/app/src/inc/global_signals.h" COPYONLY)
  endif()  

endif()

# Include SEGGER RTT
if(USE_SEGGER_RTT)
  if(NOT IS_DIRECTORY ${PROJECT_SOURCE_DIR}/libraries/middlewares/seggerRTT)
  message("Clone repo SeggerRTT...")
  execute_process(
    COMMAND git submodule add https://github.com/eas-engineering/seggerRTT.git libraries/middlewares/seggerRTT
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
  endif()
  add_subdirectory(libraries/middlewares/seggerRTT)
endif()

# Include LwMem library
if(USE_LWMEM)
  if(NOT IS_DIRECTORY ${PROJECT_SOURCE_DIR}/libraries/middlewares/lwmem)
    message("Clone repo LwMem...")
    execute_process(
      COMMAND git submodule add https://github.com/eas-engineering/lwmem.git libraries/middlewares/lwmem
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
  endif()
  add_subdirectory(libraries/middlewares/lwmem)
endif()

# Include LwRB library
if(USE_LWRB)
  if(NOT IS_DIRECTORY ${PROJECT_SOURCE_DIR}/libraries/middlewares/lwrb)
    message("Clone repo LwRB...")
    execute_process(
      COMMAND git submodule add https://github.com/eas-engineering/ringbuff.git libraries/middlewares/lwrb
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
  endif()
  add_subdirectory(libraries/middlewares/lwrb)
endif()

if(USE_LW_UTILITY)
  if(NOT IS_DIRECTORY ${PROJECT_SOURCE_DIR}/libraries/middlewares/lw_utility)
  message("Clone repo LW Utility...")
  execute_process(
    COMMAND git submodule add https://github.com/eas-engineering/lw_utility.git libraries/middlewares/lw_utility
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
  endif()
  add_subdirectory(libraries/middlewares/lw_utility)
endif()

if(USE_LW_MATH)
  if(NOT IS_DIRECTORY ${PROJECT_SOURCE_DIR}/libraries/middlewares/lw_math)
  message("Clone repo LW Math...")
  execute_process(
    COMMAND git submodule add https://github.com/eas-engineering/lw_math.git libraries/middlewares/lw_math
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
  endif()
  add_subdirectory(libraries/middlewares/lw_math)
endif()

# Include lvgl library
if(USE_LVGL)
  if(NOT IS_DIRECTORY ${PROJECT_SOURCE_DIR}/libraries/middlewares/lvgl)
    message("Clone repo LVGL...")
    execute_process(
      COMMAND git submodule add https://github.com/eas-engineering/lvgl libraries/middlewares/lvgl
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
  endif()
  set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL TRUE)
  set(LV_CONF_BUILD_DISABLE_EXAMPLES TRUE)
  set(LV_CONF_BUILD_DISABLE_DEMOS TRUE)
  set(LV_CONF_PATH ${ProjDirPath}/app/src/inc/lv_conf.h CACHE STRING "" FORCE)
  set(LVGL_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/libraries/middlewares/lvgl)
  include(libraries/middlewares/lvgl/env_support/cmake/custom.cmake)
endif()

# Include Modbus Library
if(USE_MODBUS)
  if(NOT IS_DIRECTORY ${PROJECT_SOURCE_DIR}/libraries/middlewares/modbus)
  message("Clone repo Modbus...")
  execute_process(
    COMMAND git submodule add https://github.com/eas-engineering/modbus.git libraries/middlewares/modbus
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
  endif()
  add_subdirectory(libraries/middlewares/modbus)
endif()

# Include Flash DB Library
if(USE_FLASH_DB)
  if(NOT IS_DIRECTORY ${PROJECT_SOURCE_DIR}/libraries/middlewares/flash_db)
  message("Clone repo Flash DB...")
  execute_process(
    COMMAND git submodule add https://github.com/eas-engineering/FlashDB.git libraries/middlewares/flash_db
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
  endif()
  if (NOT EXISTS "${PROJECT_SOURCE_DIR}/app/src/inc/fdb_cfg.h")
    configure_file("${PROJECT_SOURCE_DIR}/libraries/middlewares/flash_db/inc/fdb_cfg_template.h" "${PROJECT_SOURCE_DIR}/app/src/inc/fdb_cfg.h" COPYONLY)
  endif()
  if (NOT EXISTS "${PROJECT_SOURCE_DIR}/app/src/inc/fal_cfg.h")
    configure_file("${PROJECT_SOURCE_DIR}/libraries/middlewares/flash_db/port/fal/samples/porting/fal_cfg.h" "${PROJECT_SOURCE_DIR}/app/src/inc/fal_cfg.h" COPYONLY)
  endif()     
  add_subdirectory(libraries/middlewares/flash_db)
endif()


# Elimino dalla lista tutti i file che contengono la parola "template"
list(FILTER SRC_FILES EXCLUDE REGEX ".*template.*")

# Includo nel progetto i file contenuti nella cartella src
target_sources(${PROJECT_NAME} PRIVATE
  ${SRC_FILES}
)

if(USE_SEGGER_RTT)
target_link_libraries(${PROJECT_NAME} seggerRTT)
endif()

if(USE_LWMEM)
  target_link_libraries(${PROJECT_NAME} lwmem)
endif()

if(USE_LWRB)
  target_link_libraries(${PROJECT_NAME} lwrb)
endif()

if(USE_LW_UTILITY)
  target_link_libraries(${PROJECT_NAME} lwutility)
endif()

if(USE_LW_MATH)
  target_link_libraries(${PROJECT_NAME} lwmath)
endif()

if(USE_LVGL)
  target_link_libraries(${PROJECT_NAME} lvgl)
  #target_link_libraries(${PROJECT_NAME} lvgl_demos)
  #target_link_libraries(${PROJECT_NAME} lvgl_examples)
endif()

if(USE_MODBUS)
  target_link_libraries(${PROJECT_NAME} modbus)
endif()

if(USE_FLASH_DB)
  target_link_libraries(${PROJECT_NAME} flash_db)
endif()

# Include il framework QPC per la gestione delle macchine a stati
if(USE_QPC)

  if (NOT EXISTS "${PROJECT_SOURCE_DIR}/app/src/inc/global_signals.h")
  configure_file("${PROJECT_SOURCE_DIR}/libraries/middlewares/qpc/template/global_signals.h" "${PROJECT_SOURCE_DIR}/app/src/inc/global_signals.h" COPYONLY)
  endif()

  if (NOT EXISTS "${PROJECT_SOURCE_DIR}/libraries/drivers/BSP/src/bsp_qpc.c")
  configure_file("${PROJECT_SOURCE_DIR}/libraries/middlewares/qpc/template/BSP/src/bsp_qpc.c" "${PROJECT_SOURCE_DIR}/libraries/drivers/BSP/src/bsp_qpc.c" COPYONLY)
  endif()

  if (NOT EXISTS "${PROJECT_SOURCE_DIR}/libraries/drivers/BSP/src/include/bsp_qpc.h")
  configure_file("${PROJECT_SOURCE_DIR}/libraries/middlewares/qpc/template/BSP/src/include/bsp_qpc.h" "${PROJECT_SOURCE_DIR}/libraries/drivers/BSP/src/include/bsp_qpc.h" COPYONLY)
  endif()  

  if (NOT EXISTS "${PROJECT_SOURCE_DIR}/libraries/drivers/BSP/src/bsp_qpc_spy.c")
  configure_file("${PROJECT_SOURCE_DIR}/libraries/middlewares/qpc/template/BSP/src/bsp_qpc_spy.c" "${PROJECT_SOURCE_DIR}/libraries/drivers/BSP/src/bsp_qpc_spy.c" COPYONLY)
  endif()

  if (NOT EXISTS "${PROJECT_SOURCE_DIR}/libraries/drivers/BSP/src/include/bsp_qpc_spy.h")
  configure_file("${PROJECT_SOURCE_DIR}/libraries/middlewares/qpc/template/BSP/src/include/bsp_qpc_spy.h" "${PROJECT_SOURCE_DIR}/libraries/drivers/BSP/src/include/bsp_qpc_spy.h" COPYONLY)
  endif()  

  if (NOT EXISTS "${PROJECT_SOURCE_DIR}/libraries/drivers/BSP/src/bsp_qpc_test.c")
  configure_file("${PROJECT_SOURCE_DIR}/libraries/middlewares/qpc/template/BSP/src/bsp_qpc_test.c" "${PROJECT_SOURCE_DIR}/libraries/drivers/BSP/src/bsp_qpc_test.c" COPYONLY)
  endif()

  if (NOT EXISTS "${PROJECT_SOURCE_DIR}/libraries/drivers/BSP/src/include/bsp_qpc_test.h")
  configure_file("${PROJECT_SOURCE_DIR}/libraries/middlewares/qpc/template/BSP/src/include/bsp_qpc_test.h" "${PROJECT_SOURCE_DIR}/libraries/drivers/BSP/src/include/bsp_qpc_test.h" COPYONLY)
  endif() 

  # Include QPC Framework
  set(QPC_CFG_QPCONFIG_H_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/app/src/inc/)
  include(${QPC_SDK_PATH}/qpc_sdk_init.cmake)
  qpc_sdk_init()

  if(QPC_CFG_INTEGRATION_TEST AND CMAKE_BUILD_TYPE STREQUAL "Spy")
      add_compile_definitions(-DQ_UTEST=0)
  endif()

  target_link_libraries(${PROJECT_NAME} qpc)
endif()

# Execute post-build to print size, generate hex and bin
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
)
