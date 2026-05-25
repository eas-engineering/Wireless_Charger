/*****************************************************************************
* Filename              :   modbus_server_manager.h
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   21 March 2025
*
* Copyright (c) 2024 EAS SPA. All rights reserved.
*
******************************************************************************/

/** @file modbus_server_manager.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef MODBUS_SERVER_MANAGER_H_
#define MODBUS_SERVER_MANAGER_H_

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

/**
 * @brief Signal definition for drive events management
 * 
 */
enum {
  MODBUS_PASSWORD_OK_SIG = MODBUS_SERVER_GROUP, /**< Motor connecting event */
  MODBUS_WRITE_VALUE_SIG,
  MODBUS_NET_EVENT_SIG,
  MODBUS_UNBLOCKED_SIG,
  MODBUS_GROUP_MAX_SIG
};

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/
typedef struct {
    QEvt super;
    /* V*100 -> 17.00V = 1700 */      
    uint32_t vbat;
    /* mA */   
    uint32_t ibat;   
    uint32_t tbat;   
    uint32_t soc_cc;
    /* in minutes */
    uint32_t end_of_charge_time;
    uint32_t number_of_charges;
    uint32_t allarm;
    uint32_t mAh_n_cycles_charge;
    uint32_t mAh_tot;
} ModBusInfoEvt;

extern QActive* const AO_ModbusServerManager; // opaque pointer

/*****************************************************************************
* Function Prototypes
******************************************************************************/

void modbus_server_manager_init(void);

/**
 * }
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*MODBUS_SERVER_MANAGER_H_*/

/*** End of File *************************************************************/
