/******************************************************************************
 * Filename              : bsp_digital_output.h
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

/** @file bsp_digital_output.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef BSP_DIGITAL_OUTPUT_H_
#define BSP_DIGITAL_OUTPUT_H_

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

typedef enum {
  IO_OFF = 0,
  IO_ON = 1,
} io_state_t;

typedef enum {
  IO_V_AUX_EN = 0,
  IO_BAT_SW_EN = 1,
  IO_CH_PWM_SYNC = 2,
} io_id_t;

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*****************************************************************************
* Function Prototypes
******************************************************************************/

void bsp_digital_output_init(void);

void bsp_digital_output_set(io_id_t id, io_state_t state);

/**
 * }
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*BSP_DIGITAL_OUTPUT_H_*/

/*** End of File *************************************************************/
