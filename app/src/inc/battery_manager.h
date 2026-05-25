/*****************************************************************************
* Filename              :   battery_manager.h
* Author                :   Giulio Nardon
* Origin Date           :   25 March 2026
*
* Copyright (c) 2026 EAS Engineering srl. All rights reserved.
*
******************************************************************************/

/** @file battery_manager.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef BATTERY_MANAGER_H_
#define BATTERY_MANAGER_H_

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdint.h>
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

typedef struct {
  QEvt super;
  uint16_t ibat_mm;
  uint16_t vbat_mm;
  bool isCharging;
  float mAh;
  float mAh_n_cycles_charge;
  bool first_cycle;
} DatabaseEvt;

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*****************************************************************************
 * @brief Initializes the battery manager module.
 *
 * @details This function is responsible for initializing the battery manager module.
 *        It should be called before any other function in this module.
 *
 * @return None
 *****************************************************************************/
void battery_manager_init(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*BATTERY_MANAGER_H_*/

/*** End of File *************************************************************/
