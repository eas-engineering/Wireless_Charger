/*****************************************************************************
* Filename              :   global_signals.h
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   27 August 2024
*
* Copyright (c) 2024 EAS SPA. All rights reserved.
*
******************************************************************************/

/** @file global_signals.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef GLOBAL_SIGNALS_H_
#define GLOBAL_SIGNALS_H_

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdint.h>
#include "qpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup        GlobalSignals
 * \brief           This groups rappresents the list of the signals used by
 *                  the system
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

enum GlobalSignals {
  DUMMY_SIG = Q_USER_SIG,
  ADC_BATTERY_INFO_SAMPLE_SIG,
  EEPROM_WRITE_SIG,
  INITIALIZE_SIG,
  BUTTON_PRESSED_SIG,
  MAX_PUB_SIG, // the last published signal

  ALARM_SIG,
  ADC_DATA_READY_SIG,
  TIMEOUT_SIG,

  BSP_I2C_GROUP,
  BSP_I2C_GROUP_END = BSP_I2C_GROUP + 20U,

  MAX_SIG // the last signal
};

/*****************************************************************************
* Function Prototypes
******************************************************************************/

#ifdef Q_SPY
static inline void
produce_sig_dict(void) {
  QS_SIG_DICTIONARY(DUMMY_SIG, (void*)0);
  QS_SIG_DICTIONARY(TIMEOUT_SIG, (void*)0);
}
#endif // def Q_SPY

/**
 * }
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*GLOBAL_SIGNALS_H_*/

/*** End of File *************************************************************/
