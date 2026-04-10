/******************************************************************************
 * Filename              : bsp_tick.c
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

/** @file bsp_tick.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include "bsp_tick.h"
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

static volatile uint32_t uwTick;

/*****************************************************************************
* Function Definitions
******************************************************************************/

/**
 * @brief      Initialize the tick counter
 * @details   This function initializes the tick counter with a given frequency.
 *              The frequency is given in Hz and the function sets the SysTick
 *              register accordingly.
 */
void
bsp_tick_init(void) {
  // is already configured by QPC
}

/**
 * @brief      Increment the tick counter
 * @details   This function increments the tick counter at each tick interrupt.
 */
void
bsp_tick_inc(void) {
  uwTick++;
}

/**
 * @brief      Get the current tick count
 * @return     The current tick count
 * @details   This function returns the current tick count, which is
 *              incremented at each tick interrupt.
 */
uint32_t
bsp_tick_get(void) {
  return uwTick;
}