/*****************************************************************************
* Filename              :   bsp_serial.h
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   25 September 2024
*
* Copyright (c) 2024 EAS SPA. All rights reserved.
*
******************************************************************************/

/** @file bsp_serial.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef BSP_SERIAL_H_
#define BSP_SERIAL_H_

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdint.h>
#include "global_signals.h"

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

#define BSP_DRIVE_SERIAL_BUFF_LEN (32U)

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

/*****************************************************************************
* Module Typedefs
******************************************************************************/

/* Enumerations for serial interface signals */
enum {
  SERIAL_SEND_SIG = DRIVER_SERIAL_GROUP, /* Signal used when you want to send data */
  SERIAL_RECEIVE_SIG,                    /* Signal used when data has been received */
  SERIAL_RX_CMPL_SIG,                    /* Signal used when the reception is complete */
  SERIAL_TX_CMPL_SIG,                    /* Signal used when the USART transmission is complete */
};

/**
 * @brief Struct that rappresents a USART event
 * 
 */
typedef struct {
  QEvt super;
  uint8_t pui8_data[BSP_DRIVE_SERIAL_BUFF_LEN];
  uint32_t len;
} DriveSerialEvt_t;

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*****************************************************************************
* Function Prototypes
******************************************************************************/

QHsm* bsp_drive_serial_init(QActive* const container);
void bsp_drive_serial_isr_rx_handler(void);
/**
 * }
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*BSP_SERIAL_H_*/

/*** End of File *************************************************************/
