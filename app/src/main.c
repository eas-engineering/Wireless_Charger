/******************************************************************************
 * Filename              : main.c
 * Author                : Giulio Dalla Vecchia
 * Origin Date           : 29 December 2025
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

/** @file main.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include "bsp_adc.h"
#include "bsp_clock.h"
#include "bsp_pwm.h"
#include "bsp_i2c.h"
#include "bsp_stwlc_driver.h"
#include "project_settings.h"
#include "battery_manager.h"

#if defined(USE_QPC)
Q_DEFINE_THIS_FILE // define the name of this file for assertions
#endif

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

#if defined(USE_QPC)
  static QF_MPOOL_EL(BspI2CEvt_t) smlPoolSto[10];
static QSubscrList subscrSto[MAX_PUB_SIG];
#endif

/*****************************************************************************
* Function Definitions
******************************************************************************/

int
main(void) {

  bsp_clock_init();

  QF_init();

  // initialize the QS software tracing...
  if (!QS_INIT((void*)0)) {
    Q_ERROR();
  }

  // dictionaries...
#ifdef Q_SPY
  QS_OBJ_DICTIONARY(&l_SysTick_Handler);
#endif
  QS_ONLY(produce_sig_dict());

  // setup the QS filters...
  QS_GLB_FILTER(QS_ALL_RECORDS);   // all records
  QS_GLB_FILTER(-QS_QF_TICK);      // exclude
  QS_GLB_FILTER(-QS_SCHED_LOCK);   // exclude
  QS_GLB_FILTER(-QS_SCHED_UNLOCK); // exclude

#ifdef Q_UTEST
  // pause execution of the test and wait for the test script to continue
  QS_TEST_PAUSE();
#endif

  // initialize event pools
  QF_poolInit(smlPoolSto, sizeof(smlPoolSto), sizeof(smlPoolSto[0]));

  // initialize publish-subscribe
  QActive_psInit(subscrSto, Q_DIM(subscrSto));

  /* Initialize other modules */
  battery_manager_init();

  return QF_run(); // run the QF application
}
