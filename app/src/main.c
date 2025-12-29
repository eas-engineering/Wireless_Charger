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
#include "project_settings.h"

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
static QF_MPOOL_EL(QEvt) smlPoolSto[10];
static QSubscrList subscrSto[MAX_PUB_SIG];
#endif

/*****************************************************************************
* Function Definitions
******************************************************************************/

int
main(void) {

  int maj = PROJECT_VERSION_MAJOR;
  int min = PROJECT_VERSION_MINOR;
  int patch = PROJECT_VERSION_PATCH;
  int tweak = PROJECT_VERSION_TWEAK;
  char* ver = PROJECT_VERSION_STR;

#ifdef USE_EDF_FRAMEWORK
  /* Configuro le memory pool necessarie */
  edf_pool_add(&event_pool_small[0], sizeof(event_pool_small), SIZE_OF_BLOCK);
  edf_pool_add(&event_pool_medium[0], sizeof(event_pool_medium), SIZE_OF_BLOCK_MEDIUM);
  edf_pool_add(&event_pool_large[0], sizeof(event_pool_large), SIZE_OF_BLOCK_LARGE);

  /* Configure the publish-subscribe module */
  edf_ps_init(&edf_subList[0], APP_MAX_PS_SIGNAL);
#elif defined(USE_QPC)
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
#endif

  /* Initialize other modules */

#ifdef USE_EDF_FRAMEWORK
  /* Start the RTOS scheduler */
  return edf_start();
#elif defined(USE_QPC)
  return QF_run(); // run the QF application
#endif
}
