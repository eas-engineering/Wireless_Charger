/******************************************************************************
 * Filename              : bsp_led.h
 * Author                : Giulio Dalla Vecchia
 * Origin Date           : 10 April 2026
 *
 * Copyright (c) 2025 EAS Engineering srl.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ******************************************************************************/

/** @file bsp_led.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef BSP_LED_H_
#define BSP_LED_H_

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdint.h>

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

typedef enum { LED_OFF = 0, LED_ON = 1 } led_state_t;

typedef enum { LED_RED = 0, LED_GREEN = 1, LED_BLUE = 2 } led_color_t;

typedef enum { RED = 0, GREEN = 1, BLUE = 2, WHITE = 3, YELLOW = 4, OFF = 5 } rgb_color_t;

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*****************************************************************************
* Function Prototypes
******************************************************************************/

void bsp_led_init(void);

void bsp_single_led_set(led_color_t color, led_state_t state);

void bsp_color_rgb_set(rgb_color_t color);

/**
 * }
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*BSP_LED_H_*/

/*** End of File *************************************************************/
