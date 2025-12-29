message("config.cmake files is processing...")

# Imposto il nome del progetto che diventerà il target
set(PROJECT_NAME              my_project)

set(VERSION_MAJOR             1)
set(VERSION_MINOR             0)
set(VERSION_PATCH             0)
set(VERSION_TWEAK             0)
set(VERSION_STR               "v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-rc.${VERSION_TWEAK}")

# Impostare il microcontrollore utilizzato
set(USE_MCXA142               ON)

# Include SEGGER RTT
set(USE_SEGGER_RTT            OFF)

# Include LwMem library
set(USE_LWMEM                 OFF)

# Include LwRB library
set(USE_LWRB                  OFF)

# Include LW Utility
set(USE_LW_UTILITY            OFF)

# Include LVGL library
set(USE_LVGL                  OFF)

# Include LW Math
set(USE_LW_MATH               OFF)

# Include Modbus Framework
set(USE_MODBUS                OFF)

# Include Modbus Framework
set(USE_FLASH_DB              OFF)

# Include QPC Framework
set(USE_QPC                   OFF)
set(QPC_PROJECT               ${PROJECT_NAME})
set(QPC_CFG_KERNEL            QK)
set(QPC_CFG_GUI               OFF)
set(QPC_CFG_QSPY              OFF) # add QSPY support? (ON/OFF)
set(QPC_CFG_UNIT_TEST         OFF) # add unit test support? (ON/OFF)
set(QPC_CFG_INTEGRATION_TEST  OFF) # add integration test support? (ON/OFF)
set(QPC_CFG_PORT              arm-cm)