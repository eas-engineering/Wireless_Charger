/*****************************************************************************
* Filename              :   database_manager.h
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   27 March 2025
*
* Copyright (c) 2024 EAS SPA. All rights reserved.
*
******************************************************************************/

/** @file database_manager.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef DATABASE_MANAGER_H_
#define DATABASE_MANAGER_H_

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdint.h>
#include "global_signals.h"
#include "battery_nv_params.h"

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
 * @brief Signal definition for database events management
 * 
 */
enum {
  ENV_DATABASE_WRITE_SIG = DATABASE_GROUP,
  NAMEPLATE_DATABASE_WRITE_SIG,
  DATABASE_LOG_EVENT_SAVE_SIG,
  DATABASE_LOG_EVENT_READ_SIG,
  DATABASE_MAINTENANCE_EVENT_SAVE_SIG,
  DATABASE_MAINTENANCE_EVENT_READ_SIG,
  DATABASE_UPDATE_WORKING_TIME_SIG,
  DATABASE_MAX_SIG,
};

/**
 * Tipo di operazione possibile relativamente ad un parametro
 */
typedef enum { DB_NO_OP, DB_WRITING } db_operation_t;

typedef struct {
  QEvt super;

  union param_union {

    struct battery_params {

      struct kv_params {
        uint16_t value[BATTERY_PARAM_COUNT];
        db_operation_t operation[BATTERY_PARAM_COUNT];
      } kv;

      struct datalog_params {
        int16_t min;
        int16_t max;
      } datalog;

    } battery;

  } params;

} nv_params_evt_t;

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

extern QActive* const AO_DatabaseManager; // opaque pointer

/*****************************************************************************
* Function Prototypes
******************************************************************************/

void database_manager_init(void);

nv_params_evt_t* database_alloc_new_event(QSignal signal);

/**
 * }
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*DATABASE_MANAGER_H_*/

/*** End of File *************************************************************/


