include_guard(GLOBAL)
message("BSP component is included.")

file(GLOB SEGGER_RTT_SOURCE_FILES_LIST 
    "${CMAKE_CURRENT_LIST_DIR}/src/bsp_clock.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/bsp_pwm.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/bsp_adc.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/bsp_i2c.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/bsp_qpc.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/bsp_stwlc_driver.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/bsp_input.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/bsp_input_dc.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/bsp_button.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/bsp_keypad.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/bsp_timer.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/bsp_led.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/bsp_digital_output.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/bsp_tick.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/bsp_ntc_battery.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/bsp_drive_serial.c"
)

target_sources(${PROJECT_NAME} PRIVATE
	${SEGGER_RTT_SOURCE_FILES_LIST}
)

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/src/include
)
