/******************************************************************************
 * Filename              : bsp_i2c.h
 * Author                : Giulio Dalla Vecchia
 * Origin Date           : 30 December 2025
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

/** @file bsp_i2c.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef BSP_I2C_H_
#define BSP_I2C_H_

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdint.h>
#include "bsp_eeprom.h"
#include "global_signals.h"
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

typedef enum { I2C_NO_ERROR = 0, I2C_ERROR } i2c_error_t;

typedef enum { I2C_MEM_ADDR_8, I2C_MEM_ADDR_16 } i2c_mem_addr_t;

typedef enum { I2C_STATE_READY = 0, I2C_STATE_BUSY } i2c_state_t;

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*****************************************************************************
* Function Prototypes
******************************************************************************/

void bsp_i2c_init(void);

i2c_error_t bsp_i2c_writeBytes(uint8_t deviceAddress, i2c_mem_addr_t memAddrType, uint16_t memAddress,
                               uint16_t numBytes, uint8_t* pData);

i2c_error_t bsp_i2c_readByte(uint8_t deviceAddress, uint16_t memAddress, uint16_t numBytes, uint8_t* pData);

i2c_error_t bsp_i2c_getState(i2c_state_t* state);

/**
 * }
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*BSP_I2C_H_*/

/*** End of File *************************************************************/
