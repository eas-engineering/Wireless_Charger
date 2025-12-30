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

/* Enumerations for serial interface signals */
enum {
  BSP_I2C_SEND_SIG = BSP_I2C_GROUP, /* Signal used when you want to send data */
  BSP_I2C_RECEIVE_SIG,              /* Signal used when data has been received */
  BSP_I2C_TX_RX_CMPL_SIG,           /* Signal used when the transmission is complete */
  BSP_I2C_TX_RX_ERROR_SIG,          /* Signal used when an error occurs */
  BSP_I2C_TIMEOUT_SIG,              /* Signal used when a timeout occurs */
  BSP_I2C_REQ_CMPL_SIG,             /* Signal used when the request is complete */
  BSP_I2C_REQ_ERROR_SIG,            /* Signal used when an error occurs */
  BSP_I2C_MAX_SIG,
};

typedef struct {
  QEvt super;
  uint16_t DevAddress;
  uint16_t MemAddress;
  uint8_t pui8_data[128];
  uint32_t len;
  QActive* AO_sender;
} BspI2CEvt_t;

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*****************************************************************************
* Function Prototypes
******************************************************************************/

QHsm* bsp_i2c_init(QActive* const container);

/**
 * }
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*BSP_I2C_H_*/

/*** End of File *************************************************************/
