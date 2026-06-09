/******************************************************************************
 * Filename              : battery_manager.c
 * Author                : Giulio Nardon
 * Origin Date           : 23 March 2026
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

/** @file battery_manager.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include "battery_manager.h"
#include "bsp_adc.h"
#include "bsp_clock.h"
#include "bsp_digital_output.h"
#include "bsp_eeprom.h"
#include "bsp_i2c.h"
#include "bsp_keypad.h"
#include "bsp_led.h"
#include "bsp_ntc_battery.h"
#include "bsp_pwm.h"
#include "bsp_stwlc_driver.h"
#include "database_manager.h"
#include "fsl_wwdt.h"
#include "global_signals.h"
#include "modbus_server_manager.h"
#include "project_settings.h"
#include "qpc.h"
#include "soc_NiMh.h"

#if defined(USE_QPC)
Q_DEFINE_THIS_FILE // define the name of this file for assertions
#endif

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/
#define IBAT_NOISE_MA              10    // soglia rumore
#define IBAT_MAX_MA                800   // protezione software
#define IBAT_ALPHA                 0.05f // coefficiente EMA 0.2
#define IBAT_WARMUP_SAMPLES        5     // 5 * 0.5s = 2.5s

#define MAX_CHARGE_CURRENT         950
#define ON_CHARGE_CURRENT          475
#define TRICKLE_CHARGE_CURRENT     75
#define MAX_BATTERY_TEMPERATURE_mC 4500

#define MIN_BATTERY_SOC            20
#define MIN_BATTERY_VOLTAGE_V      14
#define MAX_BATTERY_VOLTAGE_V      18

#define VOLTAGE_RATIO              1000U / 1525U
#define CURRENT_RATIO              12500U /* (1/80mV)*1000 fattore di conversione sensore in mA */
#define OFFSET_CURR_SENSOR         2U     /*La corrente iniziale non è 0, ma 9mA.. ne tengo conto*/

#define SEPIC_EFF                  26U /* Efficienza del convertitore SEPIC, da misurare sperimentalmente */
#define MAX_WLC_POWER_W       15U  /* Potenza massima erogabile dal caricatore wireless, da misurare sperimentalmente */
#define END_CHARGE_CURRENT_MA 150U /* Corrente di fine carica, da misurare sperimentalmente */
#define MAX_SEPIC_POWER_W     11U  /* Potenza massima erogabile dal convertitore SEPIC, da misurare sperimentalmente */
#define VCC_HALF_MV           1650U
#define VCC_MV                3270U
#define EEPROM_SAVE_TIME_MS   60 * 1000 * 5
#define SECOND_N_TICKS        1000U
#define N_MOVING_AVERAGE_SAMPLES 5U

  /*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

  /*****************************************************************************
* Module Typedefs
******************************************************************************/

  typedef struct {
  uint16_t soc_v;
  uint16_t soc_cc;
  uint16_t soc;
  float mAh;
  float mAh_neg;
  float mAh_n_cycles_charge;
  float mAh_tot;
  uint16_t vbat_raw;
  uint16_t ibat_raw;
  int16_t tbat_raw;
  uint16_t ibat_history[N_MOVING_AVERAGE_SAMPLES];
  uint16_t vbat_history[N_MOVING_AVERAGE_SAMPLES];
  int16_t tbat_history[N_MOVING_AVERAGE_SAMPLES];
  uint16_t zero_current_history[N_MOVING_AVERAGE_SAMPLES];
  uint8_t filter_index;
  uint8_t sample_count;
  uint32_t ibat_adc;
  uint32_t ibat_sum;
  uint32_t vbat_sum;
  int32_t tbat_sum;
  uint32_t zero_current_sum;
  uint16_t ibat_mm;
  uint16_t vbat_mm;
  int16_t tbat_mm;
  uint16_t zero_current_value_mm;
  float curr_lp;
} batteryInfo_t;

typedef struct {
  uint16_t soc_db;
  uint16_t mAh_db;
  uint16_t mAh_n_cycles_charge_db;
  uint16_t mAh_tot_db;
  uint16_t n_cycles_db;
  uint16_t zero_current_value_db;
  bool first_cycle_db;
} database_value_t;

// Active Object
typedef struct {
  QActive super;
  QTimeEvt timerEvt;

  batteryInfo_t battInfo;
  database_value_t databaseInfo;

  bool LedIsOn;
  bool moving_average_initialized;
  bool isCharging;
  bool endCharge;
  uint16_t termination_voltage_mV;
  uint16_t zero_current_value;

  uint32_t cnt_battery_mangager;
  uint32_t cnt_cc_charge;
  uint32_t cnt_cv_charge;
  uint32_t cnt_eeprom_data_save;

  uint16_t end_charge_time;
  bool first_cycle;
  uint16_t number_of_charges;
  bool force_off_battery;
  bool five_minutes;
  bool reset_lp_filter;

  uint16_t allarm_info;
} BatteryManager_t;

/* Fasce di temperatura (°C) */
static const int16_t temp_ranges[5][2] = {
  {-1000, 0},   // -10 ≤ T < 0
  {0, 1000},    // 0 ≤ T < 10
  {1000, 2000}, // 10 ≤ T < 20
  {2000, 3000}, // 20 ≤ T < 30
  {3000, 4500}  // 30 ≤ T < 45
};

/* Fasce di corrente (It rate) */
static const uint32_t current_ranges_mA[3][2] = {
  {100, 200}, // 0.05 It ≤ I ≤ 0.10 It
  {200, 600}, // 0.10 It ≤ I ≤ 0.30 It
  {600, 1000} // 0.30 It ≤ I ≤ 0.50 It
};

/* MATRICE DELLE TENSIONI CCV (V/cell)
   Riga = fascia temperatura
   Colonna = fascia corrente
*/
static const uint32_t termination_voltage_charge_mV[5][3] = {
  /* -10 ≤ T < 0°C */
  {1752, 1776, 1908},

  /* 0 ≤ T < 10°C */
  {1728, 1740, 1824},

  /* 10 ≤ T < 20°C */
  {1716, 1728, 1776},

  /* 20 ≤ T < 30°C */
  {1704, 1716, 1752},

  /* 30 ≤ T < 45°C */
  {1692, 1704, 1740}};

enum {
  NO_ALLARM = 0,
  BATTERY_VERY_LOW = 1,
  ANOMALOUS_BATTERY_TEMP = 2,
  NO_BATTERY_PRESENCE = 3,
  ANOMALOUS_CHARGE_CURR = 4,
  ANOMALOUS_CHARGE_VOLT = 5
};

/*****************************************************************************
* Function Prototypes
******************************************************************************/

void battery_manager_init(void);

static QState battery_manager_initial_state(BatteryManager_t* const me, void const* const par);
static QState battery_manager_initialize_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_startup_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_active_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_soft_start_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_on_cc_charge_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_on_cv_charge_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_soft_end_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_end_charge_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_on_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_high_level_batt_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_low_level_batt_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_allarm_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_delay_vsup_to_vch_state(BatteryManager_t* const me, QEvt const* const e);

static void batteryInfo_update_moving_average(batteryInfo_t* const battInfo, BatteryManager_t* const me);
static QState charger_cc_manager(BatteryManager_t* const me, batteryInfo_t battInfo, uint32_t max_charge_curr_mA);
static QState charger_cv_manager(BatteryManager_t* const me, batteryInfo_t battInfo, uint32_t max_charge_curr_mA);
static uint32_t current_to_pwm(uint32_t curr_mA);
static void batteryInfo_process_data(BatteryManager_t* me, AdcInfoEvt const* evt);
static float filter_ibat_mA(float ibat_raw_mA, BatteryManager_t* const me);

static void initWWDT(void);
/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

static BatteryManager_t BatteryManager;
QActive* const AO_BatteryManager = &BatteryManager.super;

/*****************************************************************************
* Function Definitions
******************************************************************************/

/**
 * @brief Initializes the battery manager module.
 *
 * @details This function is responsible for initializing the battery manager module.
 *        It should be called before any other function in this module.
 * @return None
 */
void
battery_manager_init(void) {
  static QEvt const* BatteryManagerQueueSto[10];
  BatteryManager_t* const me = &BatteryManager;
  QActive_ctor(&me->super, Q_STATE_CAST(&battery_manager_initial_state));
  QTimeEvt_ctorX(&me->timerEvt, &me->super, TIMEOUT_SIG, 0U);

  QACTIVE_START(AO_BatteryManager,
                4U,                            // QP prio. of the AO
                BatteryManagerQueueSto,        // event queue storage
                Q_DIM(BatteryManagerQueueSto), // queue length [events]
                (void*)0, 0U,                  // no stack storage
                (void*)0);                     // no initialization param
}

static QState
battery_manager_initial_state(BatteryManager_t* const me, void const* const par) {
  Q_UNUSED_PAR(par);
  return Q_TRAN(&battery_manager_initialize_state);
}

/**
 * @brief Initializes the battery manager module.
 *
 * @details This function is responsible for initializing the battery manager module.
 *        It should be called before any other function in this module.
 * @return None
 * @param me The active object of the battery manager module.
 * @param e The event that triggered the transition.
 */
static QState
battery_manager_initialize_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      QActive_subscribe(&me->super, BUTTON_PRESSED_SIG);
      QActive_subscribe(&me->super, ADC_BATTERY_INFO_SAMPLE_SIG);
      QActive_subscribe(&me->super, ENV_DATABASE_CHANGED_SIG);
      status = Q_HANDLED();
      break;
    }

    case ENV_DATABASE_CHANGED_SIG: {
      me->databaseInfo.mAh_db = (uint16_t)(Q_EVT_CAST(nv_params_evt_t)->params.battery.kv.value[MAH_PARAM]);
      me->battInfo.mAh = (float)me->databaseInfo.mAh_db;

      me->databaseInfo.mAh_n_cycles_charge_db =
        (uint16_t)(Q_EVT_CAST(nv_params_evt_t)->params.battery.kv.value[MAH_N_CYCLES_CHARGE_PARAM]);
      me->battInfo.mAh_n_cycles_charge = (float)me->databaseInfo.mAh_n_cycles_charge_db;

      me->databaseInfo.mAh_tot_db = (uint16_t)(Q_EVT_CAST(nv_params_evt_t)->params.battery.kv.value[MAH_TOT_PARAM]);
      me->battInfo.mAh_tot = (float)me->databaseInfo.mAh_tot_db;

      me->databaseInfo.zero_current_value_db =
        (uint16_t)(Q_EVT_CAST(nv_params_evt_t)->params.battery.kv.value[ZERO_CURR_VAL_PARAM]);
      me->battInfo.zero_current_value_mm = me->databaseInfo.zero_current_value_db;

      me->databaseInfo.first_cycle_db =
        (uint16_t)(Q_EVT_CAST(nv_params_evt_t)->params.battery.kv.value[FIRST_CYCLE_PARAM]);
      me->first_cycle = (bool)(me->databaseInfo.first_cycle_db);

      me->databaseInfo.n_cycles_db = (uint16_t)(Q_EVT_CAST(nv_params_evt_t)->params.battery.kv.value[N_CYCLES_PARAM]);
      me->number_of_charges = me->databaseInfo.n_cycles_db;

      static const QEvt evt = QEVT_INITIALIZER(INITIALIZE_SIG);
      QACTIVE_POST(AO_BatteryManager, &evt, 0U);
      status = Q_HANDLED();
      break;
    }

    case INITIALIZE_SIG: {
      bsp_pwm_init();
      bsp_digital_output_init();
      bsp_led_init();
      keypad_init();
      bsp_adc_init();
      status = Q_TRAN(&battery_manager_startup_state);
      break;
    }

    case Q_EXIT_SIG: {
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

/**
 * @brief The active state of the battery manager module.
 * @details This state is responsible for managing the battery charging/discharging process.
 *        It will transition to the alarm state upon receiving the ALARM_SIG event.
 *        It will transition back to the top state upon receiving the Q_EXIT_SIG event.
 */
static QState
battery_manager_active_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      status = Q_HANDLED();
      break;
    }

    case ALARM_SIG: {
      status = Q_TRAN(&battery_manager_allarm_state);
      break;
    }

    case ENV_DATABASE_CHANGED_SIG: {
      me->databaseInfo.mAh_db = (uint16_t)(Q_EVT_CAST(nv_params_evt_t)->params.battery.kv.value[MAH_PARAM]);
      me->databaseInfo.mAh_n_cycles_charge_db =
        (uint16_t)(Q_EVT_CAST(nv_params_evt_t)->params.battery.kv.value[MAH_N_CYCLES_CHARGE_PARAM]);
      me->databaseInfo.mAh_tot_db = (uint16_t)(Q_EVT_CAST(nv_params_evt_t)->params.battery.kv.value[MAH_TOT_PARAM]);
      me->databaseInfo.n_cycles_db = (uint16_t)(Q_EVT_CAST(nv_params_evt_t)->params.battery.kv.value[N_CYCLES_PARAM]);
      me->databaseInfo.zero_current_value_db =
        (uint16_t)(Q_EVT_CAST(nv_params_evt_t)->params.battery.kv.value[ZERO_CURR_VAL_PARAM]);
      me->databaseInfo.first_cycle_db =
        (uint16_t)(Q_EVT_CAST(nv_params_evt_t)->params.battery.kv.value[FIRST_CYCLE_PARAM]);
      if (me->force_off_battery) {
        bsp_digital_output_set(IO_BAT_SW_EN, IO_OFF);
      }
      status = Q_HANDLED();
      break;
    }

    case ADC_BATTERY_INFO_SAMPLE_SIG: {
      /* refresh watchdog */
      WWDT_Refresh(WWDT0);

      batteryInfo_process_data(me, Q_EVT_CAST(AdcInfoEvt));
      static QEvt const evt = QEVT_INITIALIZER(ADC_DATA_READY_SIG);
      QACTIVE_POST(AO_BatteryManager, &evt, 0U);

      ModBusInfoEvt* evtModBus = Q_NEW(ModBusInfoEvt, MODBUS_BATTERY_INFO_UPDATE_SIG);
      evtModBus->vbat = me->battInfo.vbat_mm;
      evtModBus->ibat = me->battInfo.curr_lp;
      evtModBus->tbat = me->battInfo.tbat_mm;
      evtModBus->number_of_charges = me->number_of_charges;
      evtModBus->soc_cc = me->battInfo.soc_cc;
      evtModBus->end_of_charge_time = me->end_charge_time;
      evtModBus->allarm = me->allarm_info;
      evtModBus->mAh_n_cycles_charge = me->battInfo.mAh_n_cycles_charge;
      evtModBus->mAh_tot = me->battInfo.mAh_tot;
      QACTIVE_POST(AO_ModbusServerManager, &evtModBus->super, 0U);

      DatabaseEvt* evtDatabase = Q_NEW(DatabaseEvt, DATABASE_INFO_UPDATE_SIG);
      evtDatabase->ibat_mm = me->battInfo.ibat_mm;
      evtDatabase->vbat_mm = me->battInfo.vbat_mm;
      evtDatabase->isCharging = me->isCharging;
      evtDatabase->mAh = me->battInfo.mAh;
      evtDatabase->mAh_n_cycles_charge = me->battInfo.mAh_n_cycles_charge;
      QACTIVE_POST(AO_DatabaseManager, &evtDatabase->super, 0U);

      /* se ho finito la carica, ma il dispositivo rimane collegato al caricatore e si scarica, faccio ripartire la carica */
      if (me->endCharge && me->battInfo.mAh < 1590U) {
        me->endCharge = false;
        me->isCharging = true;
        status = Q_TRAN(&battery_manager_soft_start_state);
        break;
      }
      status = Q_HANDLED();
      break;
    }

    case Q_EXIT_SIG: {
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

/**
 * @brief State of the battery manager module.
 * @details This state is responsible for initializing the battery manager module.
 *        It should be called before any other function in this module.
 * @return None
 */
static QState
battery_manager_startup_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      me->allarm_info = NO_ALLARM;
      me->moving_average_initialized = false;
      me->endCharge = false;
      me->zero_current_value = me->databaseInfo.zero_current_value_db;
      me->battInfo.filter_index = 0;
      me->battInfo.sample_count = 0;
      me->battInfo.ibat_sum = 0;
      me->battInfo.vbat_sum = 0;
      me->battInfo.tbat_sum = 0;
      me->battInfo.zero_current_sum = 0;
      me->battInfo.mAh_neg = 0;
      for (uint8_t i = 0; i < N_MOVING_AVERAGE_SAMPLES; i++) {
        me->battInfo.ibat_history[i] = 0;
        me->battInfo.vbat_history[i] = 0;
        me->battInfo.tbat_history[i] = 0;
        me->battInfo.zero_current_history[i] = 0;
      }
      status = Q_HANDLED();
      break;
    }
    /* il micro si accende tramite la pressione di uno di questi due pulsanti */
    case BUTTON_PRESSED_SIG: {
      if (Q_EVT_CAST(keypad_event_t)->btn == BSP_KEYPAD_CHARGE_BTN) {
        if (Q_EVT_CAST(keypad_event_t)->event == BSP_BUTTON_ONPRESSED_EVENT) {
          /* attendo me->moving_average_initialized per tarare il sensore di corrente*/
          /* condizione lasciata solo per chiarezza lettura codice */
          me->isCharging = true;
          QActive_unsubscribe(&me->super, BUTTON_PRESSED_SIG);
        }
      } else if (Q_EVT_CAST(keypad_event_t)->btn == BSP_KEYPAD_ON_OFF_BTN) {
        if (Q_EVT_CAST(keypad_event_t)->event == BSP_BUTTON_ONPRESSED_EVENT) {
          bsp_digital_output_set(IO_BAT_SW_EN, IO_ON);
          status = Q_TRAN(&battery_manager_high_level_batt_state);
          break;
        }
      }
      status = Q_HANDLED();
      break;
    }

    case ADC_DATA_READY_SIG: {
      if (me->moving_average_initialized) {
        me->zero_current_value = me->battInfo.zero_current_value_mm - OFFSET_CURR_SENSOR;
        /* tengo il valore dello zero di corrente aggiornato in EEPROM ad ogni ricarica così non devo preoccuparmi di eventuali offset nel tempo */
        nv_params_evt_t* evt = database_alloc_new_event(ENV_DATABASE_WRITE_SIG);
        evt->params.battery.kv.operation[ZERO_CURR_VAL_PARAM] = DB_WRITING;
        evt->params.battery.kv.value[ZERO_CURR_VAL_PARAM] = me->zero_current_value;
        QACTIVE_POST(AO_DatabaseManager, (QEvt*)evt, me);

        bsp_digital_output_set(IO_BAT_SW_EN, IO_OFF);
        bsp_digital_output_set(IO_V_AUX_EN, IO_ON);
        bsp_color_rgb_set(WHITE);
        QActive_unsubscribe(&me->super, ADC_BATTERY_INFO_SAMPLE_SIG);
        /* soft start wlc.. per 3 secondi non accendo il SEPIC, ma solo lo schermo */
        QTimeEvt_armX(&me->timerEvt, 3 * SECOND_N_TICKS, 0);
      }
      status = Q_HANDLED();
      break;
    }

    case TIMEOUT_SIG: {
      status = Q_TRAN(&battery_manager_soft_start_state);
      break;
    }

    case Q_EXIT_SIG: {
      QTimeEvt_disarm(&me->timerEvt);
      QActive_subscribe(&me->super, ADC_BATTERY_INFO_SAMPLE_SIG);
      status = Q_HANDLED();
      break;
    }

    default: {
      status = Q_SUPER(&battery_manager_active_state);
      break;
    }
  }
  return status;
}

static QState
battery_manager_soft_start_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      bsp_digital_output_set(IO_CH_PWM_SYNC, IO_ON);
      me->cnt_battery_mangager = 100;
      QTimeEvt_armX(&me->timerEvt, SECOND_N_TICKS, 0);
      status = Q_HANDLED();
      break;
    }

    case ADC_DATA_READY_SIG: {
      /* Verifica se le condizioni di soft-start sono soddisfatte */
      if (me->battInfo.vbat_mm < 500U) {
        me->allarm_info = NO_BATTERY_PRESENCE;
        /* Tensione o temperatura fuori range → allarme */
        status = Q_TRAN(&battery_manager_allarm_state);
        break;
      }
      if ((me->battInfo.tbat_mm >= 4500 || me->battInfo.tbat_mm < -1000)) {
        me->allarm_info = ANOMALOUS_BATTERY_TEMP;
        /* Tensione o temperatura fuori range → allarme */
        status = Q_TRAN(&battery_manager_allarm_state);
        break;
      }

      status = Q_HANDLED();
      break;
    }

    case TIMEOUT_SIG: {
      /* Incrementa gradualmente il duty cycle del PWM durante il soft-start */
      bsp_pwm_set_duty(current_to_pwm(me->cnt_battery_mangager));
      me->cnt_battery_mangager += 100U;

      if (me->cnt_battery_mangager <= 500U) {
        /* Rampa di soft-start ancora in corso */
        QTimeEvt_armX(&me->timerEvt, SECOND_N_TICKS, 0);
        status = Q_HANDLED();
      } else {
        /* Rampa completata: verifica se il SEPIC si è attivato */
        if (me->battInfo.ibat_mm < 100U) {
          /* Corrente insufficiente → batteria è carica, SEPIC non attivo */
          status = Q_TRAN(&battery_manager_end_charge_state);
        } else {
          /* Corrente sufficiente → SEPIC attivo, inizio carica CC */
          status = Q_TRAN(&battery_manager_on_cc_charge_state);
        }
      }
      break;
    }

    case Q_EXIT_SIG: {
      QTimeEvt_disarm(&me->timerEvt);
      me->cnt_battery_mangager = 0;
      status = Q_HANDLED();
      break;
    }

    default: {
      status = Q_SUPER(&battery_manager_active_state);
      break;
    }
  }
  return status;
}

/**
 * @brief State of the battery manager module.
 * @details This state is responsible for managing the battery charging process.
 *        It will transition to the trickle state upon receiving the ALARM_SIG event.
 *        It will transition back to the active state upon receiving the Q_EXIT_SIG event.
 */
static QState
battery_manager_on_cc_charge_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      bsp_color_rgb_set(BLUE);
      QTimeEvt_armX(&me->timerEvt, SECOND_N_TICKS / 2, 0);
      me->LedIsOn = true;
      status = Q_HANDLED();
      break;
    }

    case ADC_DATA_READY_SIG: {
      status = charger_cc_manager(me, me->battInfo, 450U);
      break;
    }

    case TIMEOUT_SIG: {
      QTimeEvt_armX(&me->timerEvt, SECOND_N_TICKS / 2, 0);
      if (me->LedIsOn) {
        bsp_color_rgb_set(OFF);
        me->LedIsOn = false;
      } else {
        bsp_color_rgb_set(BLUE);
        me->LedIsOn = true;
      }
      if (++me->cnt_battery_mangager >= 32400) {
        /* 4 ore e mezza in CC, se non è ancora arrivato alla tensione di fine carica, passo comunque alla fase di CV */
        status = Q_TRAN(&battery_manager_end_charge_state);
        break;
      }
      status = Q_HANDLED();
      break;
    }

    case Q_EXIT_SIG: {
      QTimeEvt_disarm(&me->timerEvt);
      me->cnt_battery_mangager = 0;
      status = Q_HANDLED();
      break;
    }

    default: {
      status = Q_SUPER(&battery_manager_active_state);
      break;
    }
  }
  return status;
}

static QState
battery_manager_on_cv_charge_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      me->five_minutes = true;
      bsp_color_rgb_set(BLUE);
      QTimeEvt_armX(&me->timerEvt, SECOND_N_TICKS / 2, 0);
      me->LedIsOn = true;
      status = Q_HANDLED();
      break;
    }

    case ADC_DATA_READY_SIG: {
      status = charger_cv_manager(me, me->battInfo, 400U);
      break;
    }

    case TIMEOUT_SIG: {
      QTimeEvt_armX(&me->timerEvt, SECOND_N_TICKS / 2, 0);
      if (me->LedIsOn) {
        bsp_color_rgb_set(OFF);
        me->LedIsOn = false;
      } else {
        bsp_color_rgb_set(BLUE);
        me->LedIsOn = true;
      }
      if (++me->cnt_battery_mangager >= 600U) {
        /* resto al massimo 5 min in CV */
        status = Q_TRAN(&battery_manager_soft_end_state);
        break;
      }
      status = Q_HANDLED();
      break;
    }

    case Q_EXIT_SIG: {
      me->cnt_battery_mangager = 0;
      QTimeEvt_disarm(&me->timerEvt);
      status = Q_HANDLED();
      break;
    }

    default: {
      status = Q_SUPER(&battery_manager_active_state);
      break;
    }
  }
  return status;
}

/**
 * @brief State of the battery manager module.
 * @details This state is responsible for managing the battery trickle charging process.
 *        It will transition to the active state upon receiving the Q_EXIT_SIG event.
 */
static QState
battery_manager_soft_end_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      QTimeEvt_armX(&me->timerEvt, SECOND_N_TICKS, 0);
      me->cnt_battery_mangager = 400;
      bsp_color_rgb_set(BLUE);
      status = Q_HANDLED();
      break;
    }

    case TIMEOUT_SIG: {
      /* Decrementa gradualmente il duty cycle del PWM durante il soft-end */
      bsp_pwm_set_duty(current_to_pwm(me->cnt_battery_mangager));
      me->cnt_battery_mangager -= 100U;

      if (me->cnt_battery_mangager > 100U) {
        /* Rampa di soft-end ancora in corso */
        QTimeEvt_armX(&me->timerEvt, SECOND_N_TICKS, 0);
      } else {
        status = Q_TRAN(&battery_manager_end_charge_state);
        break;
      }
      status = Q_HANDLED();
      break;
    }

    case Q_EXIT_SIG: {
      me->cnt_battery_mangager = 0;
      QTimeEvt_disarm(&me->timerEvt);
      status = Q_HANDLED();
      break;
    }

    default: {
      status = Q_SUPER(&battery_manager_active_state);
      break;
    }
  }
  return status;
}

static QState
battery_manager_end_charge_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      me->isCharging = false;
      me->endCharge = true;

      nv_params_evt_t* evt = database_alloc_new_event(ENV_DATABASE_WRITE_SIG);

      if (me->first_cycle) {
        evt->params.battery.kv.operation[FIRST_CYCLE_PARAM] = DB_WRITING;
        evt->params.battery.kv.value[FIRST_CYCLE_PARAM] = (uint16_t)0U;
      }
      /* è finito un ciclo di ricarica, quindi imposto la carica rimanente pari a 1900[mAh] */
      me->battInfo.mAh = me->battInfo.mAh_tot;
      me->battInfo.soc_cc = 100U;
      me->battInfo.soc_v = 100U;
      evt->params.battery.kv.operation[MAH_PARAM] = DB_WRITING;
      evt->params.battery.kv.value[MAH_PARAM] = me->battInfo.mAh;
      QACTIVE_POST(AO_DatabaseManager, (QEvt*)evt, me);

      QTimeEvt_armX(&me->timerEvt, 5 * SECOND_N_TICKS, 0);
      bsp_color_rgb_set(BLUE);
      status = Q_HANDLED();
      break;
    }

    case TIMEOUT_SIG: {
      if (me->cnt_battery_mangager == 0U) {
        bsp_digital_output_set(IO_CH_PWM_SYNC, IO_OFF);
        QTimeEvt_armX(&me->timerEvt, 5 * SECOND_N_TICKS, 0);
        me->cnt_battery_mangager++;
      } else if (me->cnt_battery_mangager == 1U) {
        me->cnt_battery_mangager++;
        bsp_digital_output_set(IO_V_AUX_EN, IO_OFF);
        me->reset_lp_filter = true;
      }

      status = Q_HANDLED();
      break;
    }

    case Q_EXIT_SIG: {
      me->cnt_battery_mangager = 0;
      QTimeEvt_disarm(&me->timerEvt);
      status = Q_HANDLED();
      break;
    }

    default: {
      status = Q_SUPER(&battery_manager_active_state);
      break;
    }
  }
  return status;
}

/**
 * @brief State of the battery manager module.
 * @details This state is responsible for managing the battery on charging process.
 *        It will transition to the active state upon receiving the Q_EXIT_SIG event.
 *        It will transition to the trickle charging state upon receiving the ADC_BATTERY_INFO_SAMPLE_SIG event.
 *        It will transition to the off state upon receiving the OFF_SIG event.
 */
static QState
battery_manager_on_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      /* abilito watchdog */
      initWWDT();
      me->isCharging = false;
      bsp_digital_output_set(IO_V_AUX_EN, IO_OFF);
      bsp_digital_output_set(IO_BAT_SW_EN, IO_ON);
      status = Q_HANDLED();
      break;
    }

    case BUTTON_PRESSED_SIG: {
      if (Q_EVT_CAST(keypad_event_t)->btn == BSP_KEYPAD_CHARGE_BTN) {
        if (Q_EVT_CAST(keypad_event_t)->event == BSP_BUTTON_ONPRESSED_EVENT) {
          /* delay prima di passare all'alimentazione  Vch per dare tempo al wlc di stabilizzarsi */
          status = Q_TRAN(&battery_manager_delay_vsup_to_vch_state);
          break;
        }
      } else if (Q_EVT_CAST(keypad_event_t)->btn == BSP_KEYPAD_ON_OFF_BTN) {
        if (Q_EVT_CAST(keypad_event_t)->event == BSP_BUTTON_ONPRESSED_EVENT) {
          /* turn-off board */
          nv_params_evt_t* evt = database_alloc_new_event(ENV_DATABASE_WRITE_SIG);
          evt->params.battery.kv.operation[MAH_N_CYCLES_CHARGE_PARAM] = DB_WRITING;
          evt->params.battery.kv.value[MAH_N_CYCLES_CHARGE_PARAM] = me->battInfo.mAh_n_cycles_charge;
          evt->params.battery.kv.operation[MAH_PARAM] = DB_WRITING;
          evt->params.battery.kv.value[MAH_PARAM] = me->battInfo.mAh;
          evt->params.battery.kv.operation[MAH_TOT_PARAM] = DB_WRITING;
          evt->params.battery.kv.value[MAH_TOT_PARAM] = me->battInfo.mAh_tot;
          me->force_off_battery = true;

          QACTIVE_POST(AO_DatabaseManager, (QEvt*)evt, me);
          status = Q_HANDLED();
          break;
        }
      }
      status = Q_HANDLED();
      break;
    }

    default: {
      status = Q_SUPER(&battery_manager_active_state);
      break;
    }
  }
  return status;
}

static QState
battery_manager_high_level_batt_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      bsp_color_rgb_set(GREEN);
      status = Q_HANDLED();
      break;
    }

    case ADC_DATA_READY_SIG: {
      /* se sono in idle e la tensione scende sotto i 14,6[V] vado in low level per segnalare all'utente che restano circa 5 gonfiaggi */
      uint16_t required_vbat = (me->battInfo.ibat_mm < 300U) ? (1460U) : (1300U);
      if (me->battInfo.tbat_mm >= 4500 || me->battInfo.tbat_mm < -1000) {
        status = Q_TRAN(&battery_manager_allarm_state);
        me->allarm_info = ANOMALOUS_BATTERY_TEMP;
        break;
      }
      if (me->battInfo.vbat_mm > required_vbat) {
        status = Q_HANDLED();
      } else {
        status = Q_TRAN(&battery_manager_low_level_batt_state);
      }

      if (me->battInfo.mAh_neg > 0) {
        me->battInfo.mAh_tot += me->battInfo.mAh_neg;
      }
      break;
    }

    case Q_EXIT_SIG: {
      me->cnt_battery_mangager = 0;
      QTimeEvt_disarm(&me->timerEvt);
      status = Q_HANDLED();
      break;
    }

    default: {
      status = Q_SUPER(&battery_manager_on_state);
      break;
    }
  }
  return status;
}

static QState
battery_manager_low_level_batt_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      bsp_color_rgb_set(YELLOW);
      QTimeEvt_armX(&me->timerEvt, SECOND_N_TICKS / 2, 0);
      status = Q_HANDLED();
      break;
    }

    case ADC_DATA_READY_SIG: {
      if (me->battInfo.tbat_mm >= 4500 || me->battInfo.tbat_mm < -1000) {
        status = Q_TRAN(&battery_manager_allarm_state);
        me->allarm_info = ANOMALOUS_BATTERY_TEMP;
        break;
      }
      if (me->battInfo.vbat_mm < (1300U)) {
        me->allarm_info = BATTERY_VERY_LOW;
        status = Q_TRAN(&battery_manager_allarm_state);
        break;
      }

      if (me->battInfo.mAh_neg > 0) {
        me->battInfo.mAh_tot += me->battInfo.mAh_neg;
      }
      status = Q_HANDLED();
      break;
    }
    case TIMEOUT_SIG: {
      QTimeEvt_armX(&me->timerEvt, SECOND_N_TICKS / 2, 0);
      if (me->LedIsOn) {
        bsp_color_rgb_set(OFF);
        me->LedIsOn = false;
      } else {
        bsp_color_rgb_set(YELLOW);
        me->LedIsOn = true;
      }
      status = Q_HANDLED();
      break;
    }

    case Q_EXIT_SIG: {
      me->cnt_battery_mangager = 0;
      QTimeEvt_disarm(&me->timerEvt);
      status = Q_HANDLED();
      break;
    }

    default: {
      status = Q_SUPER(&battery_manager_on_state);
      break;
    }
  }
  return status;
}

/**
 * @brief Handle events in alarm state.
 *
 * @details This function is responsible for managing the battery charging process.
 *        It will transition to the trickle state upon receiving the ALARM_SIG event.
 *        It will transition back to the active state upon receiving the Q_EXIT_SIG event.
 */
static QState
battery_manager_allarm_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      bsp_digital_output_set(IO_CH_PWM_SYNC, IO_OFF);
      bsp_color_rgb_set(RED);

      ModBusInfoEvt* evtModBus = Q_NEW(ModBusInfoEvt, MODBUS_BATTERY_INFO_UPDATE_SIG);
      evtModBus->vbat = me->battInfo.vbat_mm;
      evtModBus->ibat = me->battInfo.curr_lp;
      evtModBus->tbat = me->battInfo.tbat_mm;
      evtModBus->number_of_charges = me->databaseInfo.n_cycles_db;
      evtModBus->soc_cc = me->battInfo.soc_cc;
      evtModBus->end_of_charge_time = me->end_charge_time;
      evtModBus->allarm = me->allarm_info;
      evtModBus->mAh_n_cycles_charge = me->battInfo.mAh_n_cycles_charge;
      QACTIVE_POST(AO_ModbusServerManager, &evtModBus->super, 0U);

      /* se non è il primo ciclo e ho fatto un ciclo completo di scarica, aggiorno la caria totale */
      if (!me->first_cycle && (me->allarm_info == BATTERY_VERY_LOW)) {
        me->battInfo.mAh_tot -= me->battInfo.mAh;
        /* se ho usato più mAh rispetto a quelli stimati, aggiorno la caria totale */
        if (me->battInfo.mAh_neg > 0) {
          me->battInfo.mAh_tot += me->battInfo.mAh_neg;
        }
        nv_params_evt_t* evt = database_alloc_new_event(ENV_DATABASE_WRITE_SIG);
        evt->params.battery.kv.operation[MAH_TOT_PARAM] = DB_WRITING;
        evt->params.battery.kv.value[MAH_TOT_PARAM] = (uint16_t)me->battInfo.mAh_tot;
        QACTIVE_POST(AO_DatabaseManager, (QEvt*)evt, me);
      }

      QTimeEvt_armX(&me->timerEvt, 5 * SECOND_N_TICKS, 0);
      status = Q_HANDLED();
      break;
    }

    case BUTTON_PRESSED_SIG: {
      if (Q_EVT_CAST(keypad_event_t)->btn == BSP_KEYPAD_ON_OFF_BTN) {
        if (Q_EVT_CAST(keypad_event_t)->event == BSP_BUTTON_ONPRESSED_EVENT) {
          /* turn-off board */
          bsp_digital_output_set(IO_BAT_SW_EN, IO_OFF);
          status = Q_HANDLED();
          break;
        }
      }
      status = Q_HANDLED();
      break;
    }
    case TIMEOUT_SIG: {
      WWDT_Refresh(WWDT0);
      QTimeEvt_armX(&me->timerEvt, 5 * SECOND_N_TICKS, 0);
      if (me->allarm_info == BATTERY_VERY_LOW) {
        bsp_digital_output_set(IO_BAT_SW_EN, IO_OFF);
      }
      status = Q_HANDLED();
      break;
    }

    case Q_EXIT_SIG: {
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

static QState
battery_manager_delay_vsup_to_vch_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      nv_params_evt_t* evt = database_alloc_new_event(ENV_DATABASE_WRITE_SIG);
      evt->params.battery.kv.operation[MAH_N_CYCLES_CHARGE_PARAM] = DB_WRITING;
      evt->params.battery.kv.value[MAH_N_CYCLES_CHARGE_PARAM] = me->battInfo.mAh_n_cycles_charge;
      QTimeEvt_armX(&me->timerEvt, 6000U, 0);
      bsp_color_rgb_set(OFF);
      status = Q_HANDLED();
      break;
    }

    case TIMEOUT_SIG: {
      bsp_digital_output_set(IO_BAT_SW_EN, IO_OFF);
      status = Q_TRAN(&battery_manager_startup_state);
      break;
    }

    case Q_EXIT_SIG: {
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

static QState
charger_cc_manager(BatteryManager_t* const me, batteryInfo_t battInfo, uint32_t max_charge_curr_mA) {
  int idx_tbat, idx_ibat;
  static uint32_t charge_curr_mA;

  /* Controllo tensione minima  e massima batteria */
  if (me->battInfo.vbat_mm < (8U * 100U) || me->battInfo.vbat_mm > (18U * 100U)) {
    me->allarm_info = ANOMALOUS_CHARGE_VOLT;
    return Q_TRAN(&battery_manager_allarm_state);
  }
  /* Controllo corrente minima e massima batteria */
  if (me->battInfo.ibat_mm < 150U || me->battInfo.ibat_mm > 750U) {
    me->allarm_info = ANOMALOUS_CHARGE_CURR;
    return Q_TRAN(&battery_manager_allarm_state);
  }
  /* Temperatura fuori range → allarme */
  if (battInfo.tbat_mm >= 4500 || battInfo.tbat_mm < -1000) {
    me->allarm_info = ANOMALOUS_BATTERY_TEMP;
    return Q_TRAN(&battery_manager_allarm_state);
  }

  /* Corrente massima erogabile dal SEPIC: P = V * I */
  charge_curr_mA = (MAX_SEPIC_POWER_W * 100U * 1000U) / battInfo.vbat_mm;

  if (charge_curr_mA > max_charge_curr_mA) {
    charge_curr_mA = max_charge_curr_mA;
  }

  /* Trova indice fascia temperatura */
  for (idx_tbat = 0; idx_tbat < 5; idx_tbat++) {
    if ((battInfo.tbat_mm >= temp_ranges[idx_tbat][0]) && (battInfo.tbat_mm < temp_ranges[idx_tbat][1])) {
      break;
    }
  }

  /* Temperatura fuori range → allarme */
  if (idx_tbat >= 5) {
    return Q_TRAN(&battery_manager_allarm_state);
  }

  /* Imposta la corrente di carica */
  bsp_pwm_set_duty(current_to_pwm(charge_curr_mA));
  /* Trova indice fascia corrente (It rate) */
  for (idx_ibat = 0; idx_ibat < 3; idx_ibat++) {
    if ((charge_curr_mA >= current_ranges_mA[idx_ibat][0]) && (charge_curr_mA <= current_ranges_mA[idx_ibat][1])) {
      break;
    }
  }

  /* Corrente fuori range → allarme */
  if (idx_ibat >= 3) {
    return Q_TRAN(&battery_manager_allarm_state);
  }
  /* Imposta tensione di fine carica in base alla temperatura e alla corrente */
  me->termination_voltage_mV = termination_voltage_charge_mV[idx_tbat][idx_ibat];

  /* Stato CC */
  if (battInfo.vbat_mm <= me->termination_voltage_mV) {
    return Q_HANDLED();
  }
  /* Stato CV */
  else {
    if (me->cnt_cc_charge <= 10U) {
      me->cnt_cc_charge++;
      return Q_HANDLED();
    } else {
      return Q_TRAN(&battery_manager_on_cv_charge_state);
    }
  }
}

static QState
charger_cv_manager(BatteryManager_t* const me, batteryInfo_t battInfo, uint32_t max_charge_curr_mA) {
  /* Controllo tensione minima  e massima batteria */
  if (me->battInfo.vbat_mm < (8U * 100U) || me->battInfo.vbat_mm > (18U * 100U)) {
    me->allarm_info = ANOMALOUS_CHARGE_VOLT;
    return Q_TRAN(&battery_manager_allarm_state);
  }
  /* Controllo corrente minima e massima batteria */
  if (me->battInfo.ibat_mm < 100U || me->battInfo.ibat_mm > 750U) {
    me->allarm_info = ANOMALOUS_CHARGE_CURR;
    return Q_TRAN(&battery_manager_allarm_state);
  }
  /* Temperatura fuori range → allarme */
  if (battInfo.tbat_mm >= 4500 || battInfo.tbat_mm < -1000) {
    me->allarm_info = ANOMALOUS_BATTERY_TEMP;
    return Q_TRAN(&battery_manager_allarm_state);
  }
  /* Imposta la corrente di carica */
  bsp_pwm_set_duty(current_to_pwm(max_charge_curr_mA));
  /* fermo presto la carica in CV per preservare la batteria*/
  if (battInfo.ibat_mm >= 300U) {
    return Q_HANDLED();
  } else {
    if (me->cnt_cv_charge <= 10U) {
      me->cnt_cv_charge++;
      return Q_HANDLED();
    } else {
      return Q_TRAN(&battery_manager_end_charge_state);
    }
  }
}

static void
batteryInfo_update_moving_average(batteryInfo_t* const battInfo, BatteryManager_t* const me) {
  battInfo->ibat_sum -= battInfo->ibat_history[battInfo->filter_index];
  battInfo->ibat_history[battInfo->filter_index] = battInfo->ibat_raw;
  battInfo->ibat_sum += battInfo->ibat_raw;

  battInfo->vbat_sum -= battInfo->vbat_history[battInfo->filter_index];
  battInfo->vbat_history[battInfo->filter_index] = battInfo->vbat_raw;
  battInfo->vbat_sum += battInfo->vbat_raw;

  battInfo->tbat_sum -= battInfo->tbat_history[battInfo->filter_index];
  battInfo->tbat_history[battInfo->filter_index] = battInfo->tbat_raw;
  battInfo->tbat_sum += battInfo->tbat_raw;

  battInfo->zero_current_sum -= battInfo->zero_current_history[battInfo->filter_index];
  battInfo->zero_current_history[battInfo->filter_index] = battInfo->ibat_adc;
  battInfo->zero_current_sum += battInfo->ibat_adc;

  if (battInfo->sample_count < N_MOVING_AVERAGE_SAMPLES) {
    battInfo->sample_count += 1U;
  }

  battInfo->filter_index = (battInfo->filter_index + 1U) % N_MOVING_AVERAGE_SAMPLES;

  if (battInfo->sample_count < N_MOVING_AVERAGE_SAMPLES) {
    battInfo->ibat_mm = battInfo->ibat_raw;
    battInfo->vbat_mm = battInfo->vbat_raw;
    battInfo->tbat_mm = battInfo->tbat_raw;
    battInfo->zero_current_value_mm = battInfo->ibat_adc;
  } else {
    if (me->moving_average_initialized == false) {
      me->moving_average_initialized = true;
    }
    battInfo->ibat_mm = (uint16_t)((battInfo->ibat_sum + (N_MOVING_AVERAGE_SAMPLES - 1U)) / N_MOVING_AVERAGE_SAMPLES);
    battInfo->vbat_mm = (uint16_t)((battInfo->vbat_sum + (N_MOVING_AVERAGE_SAMPLES - 1U)) / N_MOVING_AVERAGE_SAMPLES);
    battInfo->zero_current_value_mm = (uint16_t)((battInfo->zero_current_sum + (N_MOVING_AVERAGE_SAMPLES - 1U))
                                                 / N_MOVING_AVERAGE_SAMPLES);
    battInfo->tbat_mm = (uint16_t)((battInfo->tbat_sum + (N_MOVING_AVERAGE_SAMPLES - 1U)) / N_MOVING_AVERAGE_SAMPLES);
  }
}

/**
 * @brief Converte una corrente in milliampere in un duty-cycle PWM
 *
 * La funzione converte una corrente in milliampere in un duty-cycle PWM
 * utilizzando la formula di calcolo per la tensione di uscita ViSET
 * e successivamente convertendo il valore di tensione in un duty-cycle PWM
 *
 * @param curr_mA Corrente in milliampere da convertire
 * @return Il duty-cycle PWM corrispondente alla corrente di input
 */
static uint32_t
current_to_pwm(uint32_t curr_mA) {
  if (curr_mA > 1000) {
    curr_mA = 1000;
  }

  float Ichg = curr_mA / 1000.0f; // mA → A

  /* ViSET = (I[A] * R49 + IFBVOS) * (R53 / R48) */
  float Viset = (Ichg * 0.14f + 0.0025f) * (18.00f);

  /* Converto ViSET nel duty-cycle PWM */
  float pwm = 100.0f * (Viset / 3.0f); //3.0f

  if (pwm < 0.0f) {
    pwm = 0.0f;
  }
  if (pwm > 100.0f) {
    pwm = 100.0f;
  }

  uint32_t pwm_value = (uint32_t)(pwm + 0.5f); // Arrotonda al valore intero più vicino
  return pwm_value;
}

static float
filter_ibat_mA(float ibat_raw_mA, BatteryManager_t* const me) {
  static float ibat_filt = 0.0f;
  static uint8_t sample_cnt = 0;

  // Dead-zone anti rumore
  if (ibat_raw_mA > -IBAT_NOISE_MA && ibat_raw_mA < IBAT_NOISE_MA) {
    ibat_raw_mA = 0.0f;
  }
  if (me->reset_lp_filter) {
    ibat_filt = 0.0f;
    me->reset_lp_filter = false;
  }
  // Saturazione di sicurezza
  if (ibat_raw_mA > IBAT_MAX_MA) {
    ibat_raw_mA = IBAT_MAX_MA;
  } else if (ibat_raw_mA < -IBAT_MAX_MA) {
    ibat_raw_mA = -IBAT_MAX_MA;
  }

  // Aggiorna sempre il filtro interno
  ibat_filt += IBAT_ALPHA * (ibat_raw_mA - ibat_filt);

  // Warm-up: ritorna RAW finché il filtro non è carico
  if (sample_cnt < IBAT_WARMUP_SAMPLES) {
    sample_cnt++;
    return ibat_raw_mA;
  }

  // Regime normale
  return ibat_filt;
}

static void
batteryInfo_process_data(BatteryManager_t* me, AdcInfoEvt const* evt) {
  uint32_t numerator_curr;
  uint32_t denominator_curr;

  /* valori ADC grezzi */
  me->battInfo.ibat_adc = evt->ibat;
  me->battInfo.tbat_raw = evt->tbat;
  me->battInfo.vbat_raw = evt->vbat * VOLTAGE_RATIO;

  /* Calcolo corrente batteria */
  numerator_curr = (((me->zero_current_value > evt->ibat) ? (me->zero_current_value - evt->ibat)
                                                          : (evt->ibat - me->zero_current_value))
                    * CURRENT_RATIO);

  denominator_curr = me->zero_current_value * 2U;

  /* Arrotondamento per eccesso */
  me->battInfo.ibat_raw = (uint16_t)((numerator_curr + (denominator_curr - 1U)) / denominator_curr);

  /* Media mobile */
  batteryInfo_update_moving_average(&me->battInfo, me);

  /* SoC calcolato dalla corrente */

  /* filtro passa basso per corrente */
  me->battInfo.curr_lp = filter_ibat_mA((float)me->battInfo.ibat_mm, me);

  /* calcolo mAh assorbiti o erogati */
  if (me->isCharging) {
    me->battInfo.mAh += (me->battInfo.curr_lp * 0.5f) / 3600.0f;
    me->battInfo.mAh_n_cycles_charge += (me->battInfo.curr_lp * 0.5f) / 3600.0f;
    /* questo serve per evitare di mostrare 100% fin che non è finita effettivamente la carica.. ci pensa lo stato end_charge a mostrare 100% */
    if (me->battInfo.mAh >= me->battInfo.mAh_tot - 1U) {
      me->battInfo.mAh = me->battInfo.mAh_tot - 1U;
    }
  } else {
    me->battInfo.mAh -= (me->battInfo.curr_lp * 0.5f) / 3600.0f;
    if (me->battInfo.mAh <= 0) {
      me->battInfo.mAh = -me->battInfo.mAh;
      me->battInfo.mAh_neg += me->battInfo.mAh;
      me->battInfo.mAh = 0;
    }
  }
  me->battInfo.soc_cc = soc_coulomb(me->battInfo.mAh, me->battInfo.mAh_tot, me->battInfo.tbat_mm, me->first_cycle,
                                    me->isCharging);

  /* calcolo numero cicli di ricarica */
  if (me->battInfo.mAh_n_cycles_charge >= me->battInfo.mAh_tot) {
    /* se ho finito un ciclo di carica, azzero i mAh */
    me->battInfo.mAh_n_cycles_charge = 0U;
    me->number_of_charges += 1U;
    nv_params_evt_t* evt = database_alloc_new_event(ENV_DATABASE_WRITE_SIG);
    evt->params.battery.kv.operation[N_CYCLES_PARAM] = DB_WRITING;
    evt->params.battery.kv.value[N_CYCLES_PARAM] = me->number_of_charges;
    evt->params.battery.kv.operation[MAH_N_CYCLES_CHARGE_PARAM] = DB_WRITING;
    evt->params.battery.kv.value[MAH_N_CYCLES_CHARGE_PARAM] = me->battInfo.mAh_n_cycles_charge;
    QACTIVE_POST(AO_DatabaseManager, (QEvt*)evt, me);
  }

  /* SoC calcolato dalla tensione (per ora non usato) */
  me->battInfo.soc_v = soc_from_voltage_nimh(me->battInfo.vbat_mm, me->battInfo.tbat_mm, me->battInfo.curr_lp,
                                             me->isCharging);

  if (me->isCharging) {
    /* stima del tempo di carica rimanente */
    me->end_charge_time = time_charge_estimate(me->battInfo.mAh, me->battInfo.mAh_tot, me->battInfo.curr_lp,
                                               me->five_minutes, me->first_cycle, me->battInfo.tbat_mm);
  } else {
    me->end_charge_time = 0xFFFF;
  }
}

static void
initWWDT() {
  wwdt_config_t config;
  uint32_t wdtFreq;
  /* The WDT divides the input frequency into it by 4 */
  wdtFreq = CLOCK_GetWwdtClkFreq() / 4;

  WWDT_GetDefaultConfig(&config);

  /*
      * Set watchdog feed time constant to approximately 10s
      * Set watchdog warning time to 512 ticks after feed time constant
      */
  config.timeoutValue = wdtFreq * 10;
  config.warningValue = 512;
  //config.windowValue = wdtFreq * 2;
  config.windowValue = config.timeoutValue;
  /* Configure WWDT to reset on timeout */
  config.enableWatchdogReset = true;
  /* Setup watchdog clock frequency(Hz). */
  config.clockFreq_Hz = CLOCK_GetWwdtClkFreq();
  WWDT_Init(WWDT0, &config);
}