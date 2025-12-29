# Lista di file sorgente nella cartella src
message("Main folder source files are included.")

set(MAIN_SRC ${CMAKE_CURRENT_LIST_DIR}/main.c
             ${CMAKE_CURRENT_LIST_DIR}/syscalls.c 
             ${CMAKE_CURRENT_LIST_DIR}/sysmem.c
             ${CMAKE_CURRENT_LIST_DIR}/interrupts.c)             

if(USE_MCXA142)
    set(MAIN_SRC ${MAIN_SRC}
                 ${CMAKE_CURRENT_LIST_DIR}/system_mcxa142.c
                 ${CMAKE_CURRENT_LIST_DIR}/startup_mcxa142.s)                             
endif()

# Lista di file sorgente nella cartella src
target_sources(${PROJECT_NAME} PRIVATE
    ${MAIN_SRC}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/.
    ${CMAKE_CURRENT_LIST_DIR}/inc
)