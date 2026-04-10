/*****************************************************************************
* Filename              :   bsp_keypad.h
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   19 September 2024
*
* Copyright (c) 2024 EAS SPA. All rights reserved.
*
******************************************************************************/

/** @file bsp_keypad.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef BSP_KEYPAD_H_
#define BSP_KEYPAD_H_

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdint.h>
#include "bsp_button.h"
#include "qpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup        Template
 * \brief           Template functions group
 * \{
 */

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

/*****************************************************************************
* Module Typedefs
******************************************************************************/

/**
 * @brief Enum for keypad button
 * 
 */
typedef enum {
  BSP_KEYPAD_ON_OFF_BTN = 0x0,
  BSP_KEYPAD_CHARGE_BTN,
  BSP_MAX_KEYPAD_BTN
} bsp_keypad_btn_t;

/**
 * @brief Data structure for keypad event
 * 
 */
typedef struct {
  QEvt super;
  bsp_keypad_btn_t btn;
  bsp_button_event_t event;
} keypad_event_t;

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*****************************************************************************
* Function Prototypes
******************************************************************************/

void keypad_init(void);

bsp_button_state_t keypad_get_button_state(bsp_keypad_btn_t btn);

void bsp_keypad_set_long_press_time(bsp_keypad_btn_t btn, uint32_t time);

/**
 * }
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*BSP_KEYPAD_H_*/

/*** End of File *************************************************************/
