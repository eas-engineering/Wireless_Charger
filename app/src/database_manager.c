/******************************************************************************
* Filename              :   database_manager.c
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   27 March 2025
*
* Copyright (c) 2024 EAS SPA. All rights reserved.  
*
******************************************************************************/

/** @file database_manager.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include "database_manager.h"
#include "battery_manager.h"
#include "bsp_eeprom.h"
#include "bsp_i2c.h"
#include "qpc.h"

Q_DEFINE_THIS_FILE

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/
#define SAVE_DATA_TIME 1000 * 60 * 5

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

/*****************************************************************************
* Module Typedefs
******************************************************************************/

/**
 * @brief Structure used to manage the motor manager
 * 
 */
typedef struct {
  // protected:
  QActive super; // inherit QActive

  // private:
  QTimeEvt timeEvt; // private time event generator

  bool isCharging;
  uint16_t ibat_mm;
  uint16_t vbat_mm;
  uint16_t mAh;
  uint16_t mAh_cycles;
  uint16_t n_cycles;
  uint16_t zero_current_value;
  uint16_t first_cycle;
  uint32_t cnt_debug;
} DatabaseManager_t;

/*****************************************************************************
* Function Prototypes
******************************************************************************/

static QState database_initial_state(DatabaseManager_t* const me, void const* const par);
static QState database_active_state(DatabaseManager_t* const me, QEvt const* const e);

static void databaseInfo_process_data(DatabaseManager_t* me, uint32_t delay);
/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

static DatabaseManager_t DatabaseManager;
QActive* const AO_DatabaseManager = &DatabaseManager.super;

/*****************************************************************************
* Function Definitions
******************************************************************************/

/**
 * @brief Initializes and starts the database manager active object.
 *
 * This function sets up the database manager by constructing its active object,
 * initializing a private time event, and registering the object and function
 * dictionaries for runtime analysis. It then starts the active object with
 * a specified priority and event queue storage.
 */
void
database_manager_init(void) {
  // instantiate and start AOs/threads...

  static QEvt const* DatabaseMgrQueueSto[10];

  DatabaseManager_t* const me = &DatabaseManager;
  QActive_ctor(&me->super, Q_STATE_CAST(&database_initial_state));
  QTimeEvt_ctorX(&me->timeEvt, &me->super, TIMEOUT_SIG, 0U);

  QS_OBJ_DICTIONARY(&DatabaseManager);
  QS_OBJ_DICTIONARY(&DatabaseManager.timeEvt);

  QS_FUN_DICTIONARY(&database_initial_state);
  QS_FUN_DICTIONARY(&database_active_state);

  QS_SIG_DICTIONARY(ENV_DATABASE_WRITE_SIG, (void*)0);

  QACTIVE_START(AO_DatabaseManager,
                3U,                         // QP prio. of the AO
                DatabaseMgrQueueSto,        // event queue storage
                Q_DIM(DatabaseMgrQueueSto), // queue length [events]
                (void*)0, 0U,               // no stack storage
                (void*)0);                  // no initialization param
}

nv_params_evt_t*
database_alloc_new_event(QSignal signal) {
  nv_params_evt_t* evt = Q_NEW(nv_params_evt_t, signal);

  /* Imposto a NO OPERATION la struttura che mi tiene traccia del tipo di operazione */
  for (uint32_t i = 0; i < BATTERY_PARAM_COUNT; i++) {
    evt->params.battery.kv.operation[i] = DB_NO_OP;
  }

  return evt;
}

/**
 * @brief Initializes the database manager HSM
 *
 * This state initializes the database manager by transiting to the
 * #database_active_state state. It also ignores the `par` parameter.
 * @param[in] me the database manager to initialize
 * @param[in] par unused parameter
 * @return the next state of the HSM
 */
static QState
database_initial_state(DatabaseManager_t* const me, void const* const par) {
  Q_UNUSED_PAR(par);
  bsp_i2c_init();
  bsp_eeprom_init();
  return Q_TRAN(&database_active_state);
}

/**
 * @brief Handles the active state of the database manager HSM
 *
 * This function manages the database manager when it is in its active state.
 * It processes various signals to handle initialization, database changes,
 * and writing operations. The function initializes hardware components and
 * posts initialization events. It also reads and writes database values
 * in response to specific signals and publishes events indicating changes.
 *
 * @param me Pointer to the database manager active object.
 * @param e Pointer to the event that triggered the active state.
 *
 * @return QState Transition to the next state of the database manager.
 */
static QState
database_active_state(DatabaseManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      static const QEvt evt = QEVT_INITIALIZER(INITIALIZE_SIG);
      QACTIVE_POST(AO_DatabaseManager, (QEvt*)&evt, me);

      //debug -> pulisco la eeprom ogni volta DA TOGLIERE !!!!!!!!!!!!!!!
      // nv_param_set(&battery_nv_ctx, (uint16_t)MAH_PARAM, (uint16_t)0x0000);
      // nv_param_set(&battery_nv_ctx, (uint16_t)MAH_CYCLES_PARAM, (uint16_t)0x0000);
      // nv_param_set(&battery_nv_ctx, (uint16_t)N_CYCLES_PARAM, (uint16_t)0x0000);
      // nv_param_set(&battery_nv_ctx, (uint16_t)FIRST_CYCLE_PARAM, (uint16_t)0x0001);
      // nv_param_set(&battery_nv_ctx, (uint16_t)ZERO_CURR_VAL_PARAM, (uint16_t)0x0000);

      /* ogni 5 minuti salvo in eeprom */
      QTimeEvt_armX(&me->timeEvt, SAVE_DATA_TIME, 0);
      status = Q_HANDLED();
      break;
    }

    case INITIALIZE_SIG: {

      nv_params_evt_t* evt = Q_NEW(nv_params_evt_t, ENV_DATABASE_CHANGED_SIG); // create a new event instance

      //Leggo i valori salvati nel NV per pubblicarli a tutti i sottoscrittori
      for (uint32_t i = 0; i < (uint32_t)BATTERY_PARAM_COUNT; ++i) {
        evt->params.battery.kv.value[i] = nv_param_get(&battery_nv_ctx, (uint16_t)i);
      }
      me->mAh = nv_param_get(&battery_nv_ctx, (uint16_t)MAH_PARAM);
      me->mAh_cycles = nv_param_get(&battery_nv_ctx, (uint16_t)MAH_CYCLES_PARAM);
      me->n_cycles = nv_param_get(&battery_nv_ctx, (uint16_t)N_CYCLES_PARAM);
      me->first_cycle = nv_param_get(&battery_nv_ctx, (uint16_t)FIRST_CYCLE_PARAM);
      me->zero_current_value = nv_param_get(&battery_nv_ctx, (uint16_t)ZERO_CURR_VAL_PARAM);
      QACTIVE_PUBLISH(&evt->super, 0U);

      status = Q_HANDLED();
      break;
    }

    case ENV_DATABASE_WRITE_SIG: {

      for (uint32_t i = 0; i < (uint32_t)BATTERY_PARAM_COUNT; i++) {
        if ((Q_EVT_CAST(nv_params_evt_t)->params.battery.kv.operation[i] == DB_WRITING)) {
          nv_param_set(&battery_nv_ctx, (uint16_t)i, (Q_EVT_CAST(nv_params_evt_t)->params.battery.kv.value[i]));
        }
      }
      // cast dell'evento al tipo specifico per accedere ai parametri
      nv_params_evt_t* const evt = (nv_params_evt_t* const)e;

      // Leggo i valori aggiornati (dopo la scrittura) per pubblicarli nell'evento di cambiamento
      for (uint32_t i = 0; i < (uint32_t)BATTERY_PARAM_COUNT; i++) {
        evt->params.battery.kv.value[i] = nv_param_get(&battery_nv_ctx, (uint16_t)i);
      }

      // Pubblico un evento di cambiamento dei parametri (ENV_DATABASE_CHANGED_SIG) con i nuovi valori
      evt->super.sig = ENV_DATABASE_CHANGED_SIG;
      QACTIVE_PUBLISH(&evt->super, 0U);
      /* ogni volta che viene fatto un salvataggio in eeprom faccio ripartire il timer dei salvataggi da zero */
      QTimeEvt_rearm(&me->timeEvt, SAVE_DATA_TIME);
      status = Q_HANDLED();
      break;
    }

    case DATABASE_INFO_UPDATE_SIG: {
      me->isCharging = (uint16_t)(Q_EVT_CAST(DatabaseEvt)->isCharging);
      me->ibat_mm = (uint16_t)(Q_EVT_CAST(DatabaseEvt)->ibat_mm);
      me->vbat_mm = (uint16_t)(Q_EVT_CAST(DatabaseEvt)->vbat_mm);
      me->mAh = (uint16_t)(Q_EVT_CAST(DatabaseEvt)->mAh);
      me->mAh_cycles = (uint16_t)(Q_EVT_CAST(DatabaseEvt)->mAh_cycles);
      me->first_cycle = (uint16_t)(Q_EVT_CAST(DatabaseEvt)->first_cycle);
      status = Q_HANDLED();
      break;
    }

    case TIMEOUT_SIG: {
      databaseInfo_process_data(me, SAVE_DATA_TIME);
      status = Q_HANDLED();
      break;
    }

    default: {
      status = Q_SUPER(&QHsm_top);
      break;
    }
  }
  return status;
}

static void
databaseInfo_process_data(DatabaseManager_t* me, uint32_t delay) {

  nv_params_evt_t* evt = database_alloc_new_event(ENV_DATABASE_WRITE_SIG);
  evt->params.battery.kv.operation[MAH_PARAM] = DB_WRITING;
  evt->params.battery.kv.value[MAH_PARAM] = me->mAh;
  
  if (me->isCharging) {
    evt->params.battery.kv.operation[MAH_CYCLES_PARAM] = DB_WRITING;
    evt->params.battery.kv.value[MAH_CYCLES_PARAM] = me->mAh_cycles;
  }

  QACTIVE_POST(AO_DatabaseManager, (QEvt*)evt, me);
}