/******************************************************************************
 * Filename              : bsp_tick.h
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

/** @file bsp_tick.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef BSP_TICK_H_
#define BSP_TICK_H_

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdint.h>

#ifdef __cplusplus
extern "C"{
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

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*****************************************************************************
* Function Prototypes
******************************************************************************/

void bsp_tick_init(void);

void bsp_tick_inc(void);

uint32_t bsp_tick_get(void);

/**
 * }
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*BSP_TICK_H_*/

/*** End of File *************************************************************/

