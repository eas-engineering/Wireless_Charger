/******************************************************************************
 * Filename              :   main.c
 * Author                :   Giulio Dalla Vecchia
 * Origin Date           :   01 December 2023
 *
 * Copyright (c) 2025 EAS-ENGINEERING SRL. All rights reserved.
 *
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

/**
 * @brief
 *
 * @return int32_t
 */
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

#ifdef USE_EDF_FRAMEWORK
/**
 * Funzione richiamata dal framework EDF nel caso in cui fallisce un assert
 * @param module
 * @param location
 */
void
edf_on_assert(char_t const* const module, int_t const location) {
#ifdef DEBUG
  printf("Assert FAIL: %s, riga: %d\n", module, location);
  __disable_irq();
  while (1) {};
#else
  NVIC_SystemReset();
#endif
}
#endif