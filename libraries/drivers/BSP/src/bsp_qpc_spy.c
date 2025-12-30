/******************************************************************************
* Filename              :   bsp_qpc_spy.c
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   27 August 2024
*
* Copyright (c) 2024 EAS SPA. All rights reserved.  
*
******************************************************************************/

/** @file bsp_qpc_spy.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include "bsp_qpc_spy.h"
#include "project_settings.h"
#include "qpc.h"

#ifdef Q_SPY

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

static QSTimeCtr QS_tickTime_;
static QSTimeCtr QS_tickPeriod_;

/*****************************************************************************
* Function Definitions
******************************************************************************/

/**
 * @brief 
 * 
 * @param arg 
 * @return 
 */
uint8_t
QS_onStartup(void const* arg) {
  Q_UNUSED_PAR(arg);

  static uint8_t qsTxBuf[2 * 1024]; // buffer for QS-TX channel
  QS_initBuf(qsTxBuf, sizeof(qsTxBuf));

  static uint8_t qsRxBuf[100]; // buffer for QS-RX channel
  QS_rxInitBuf(qsRxBuf, sizeof(qsRxBuf));

  QS_tickPeriod_ = SystemCoreClock / BSP_TICKS_PER_SEC;
  QS_tickTime_ = QS_tickPeriod_; // to start the timestamp at zero

  return 1U; // return success
}

/**
 * @brief 
 * 
 * @param  
 */
void
QS_onCleanup(void) {}

#ifndef Q_UTEST
/**
 * @brief 
 * 
 * @param  
 * @return 
 */
QSTimeCtr
QS_onGetTime(void) { // NOTE: invoked with interrupts DISABLED
}
#endif

/**
 * @brief No critical section in QS_onFlush() to avoid nesting of critical sections
 *        in case QS_onFlush() is called from Q_onError().
 * 
 * @param  
 */
void
QS_onFlush(void) {}

/**
 * @brief 
 *
 * @param  
 */
void
QS_onReset(void) {}

/**
 * @brief 
 *
 * @param  
 */
void
bsp_qpc_spy_tick(void) {}

/**
 * @brief 
 * 
 * @param  
 */
void
bsp_qpc_spy_process(void) {}

#ifdef Q_UTEST
//............................................................................
void
QS_onTestLoop() {}

//..........................................................................
void
QTimeEvt_tick1_(uint_fast8_t const tickRate, void const* const sender) {}
#endif

#endif // Q_SPY
