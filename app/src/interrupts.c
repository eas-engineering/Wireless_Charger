/******************************************************************************
* Filename              :   stm32_interrupts.c
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   01 August 2024
*
* Copyright (c) 2025 EAS-ENGINEERING SRL. All rights reserved.  
*
******************************************************************************/

/** @file stm32_interrupts.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include "project_settings.h"

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

/*****************************************************************************
* Module Typedefs
******************************************************************************/

/*****************************************************************************
* Function Prototypes
******************************************************************************/

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*****************************************************************************
* Function Definitions
******************************************************************************/

/**
  * @brief This function handles Non maskable interrupt.
  */
#if defined(USE_FREERTOS)
void
NMI_Handler(void) {
  while (1) {}
}
#elif defined(USE_QPC)

#else
void
NMI_Handler(void) {
  while (1) {}
}
#endif

/**
  * @brief This function handles Hard fault interrupt.
  */
void
HardFault_Handler(void) {
  while (1) {}
}

/**
  * @brief This function handles Memory management fault.
  */
void
MemManage_Handler(void) {
  while (1) {}
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void
BusFault_Handler(void) {
  while (1) {}
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void
UsageFault_Handler(void) {
  while (1) {}
}

/**
  * @brief This function handles Debug monitor.
  */
void
DebugMon_Handler(void) {}

#if defined(USE_FREERTOS)
/**
  * @brief This function handles System service call via SWI instruction.
  */
void
SVC_Handler(void) {}

/**
  * @brief This function handles Pendable request for system service.
  */
void
PendSV_Handler(void) {}

/**
  * @brief This function handles System tick timer.
  */
void
SysTick_Handler(void) {}
#elif defined(USE_QPC)
/**
  * @brief This function handles System service call via SWI instruction.
  */
void
SVC_Handler(void) {}
#else
/**
  * @brief This function handles System service call via SWI instruction.
  */
void
SVC_Handler(void) {}

/**
  * @brief This function handles Pendable request for system service.
  */
void
PendSV_Handler(void) {}

/**
  * @brief This function handles System tick timer.
  */
void
SysTick_Handler(void) {}
#endif
