/******************************************************************************
* Filename              :   bsp_qpc_test.c
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   29 August 2024
*
* Copyright (c) 2024 EAS SPA. All rights reserved.  
*
******************************************************************************/

/** @file bsp_qpc_test.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include "bsp_qpc_test.h"
#include "qpc.h"

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

/*****************************************************************************
* Function Definitions
******************************************************************************/

/**
 * @brief 
 * 
 * @param  
 */
void
QS_onTestSetup(void) {}

/**
 * @brief 
 * 
 * @param  
 */
void
QS_onTestTeardown(void) {}

/**
 * @brief host callback function to "massage" the event, if necessary
 * 
 * @param e 
 */
void
QS_onTestEvt(QEvt* e) {
#ifdef Q_HOST // is this test compiled for a desktop Host computer?
#else         // this test is compiled for an embedded Target system
#endif

  // unused parameters...
  (void)e;
}

/**
 * @brief callback function to output the posted QP events (not used here)
 * 
 * @param sender 
 * @param recipient 
 * @param e 
 * @param status 
 */
void
QS_onTestPost(void const* sender, QActive* recipient, QEvt const* e, bool status) {
  Q_UNUSED_PAR(sender);
  Q_UNUSED_PAR(recipient);
  Q_UNUSED_PAR(e);
  Q_UNUSED_PAR(status);
}

/**
 * @brief 
 *
 * @param cmdId 
 * @param param1 
 * @param param2 
 * @param param3 
 */
void
QS_onCommand(uint8_t cmdId, uint32_t param1, uint32_t param2, uint32_t param3) {
  Q_UNUSED_PAR(cmdId);
  Q_UNUSED_PAR(param1);
  Q_UNUSED_PAR(param2);
  Q_UNUSED_PAR(param3);
}
