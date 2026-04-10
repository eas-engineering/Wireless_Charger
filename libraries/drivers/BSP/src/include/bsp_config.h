/*****************************************************************************
* Filename              :   bsp_config.h
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   19 September 2024
*
* Copyright (c) 2024 EAS SPA. All rights reserved.
*
******************************************************************************/

/** @file bsp_config.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef BSP_CONFIG_H_
#define BSP_CONFIG_H_

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdint.h>
#include "bsp_keypad.h"

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

#define BSP_KEYPAD_NUM_OF_BTN (BSP_MAX_KEYPAD_BTN) /* Number of buttons */

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

/*****************************************************************************
* Module Typedefs
******************************************************************************/

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*****************************************************************************
* Function Prototypes
******************************************************************************/

/**
 * }
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*BSP_CONFIG_H_*/

/*** End of File *************************************************************/
