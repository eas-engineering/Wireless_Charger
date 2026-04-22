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
#include "bsp_i2c.h"
#include "bsp_keypad.h"
#include "bsp_led.h"
#include "bsp_ntc_battery.h"
#include "bsp_pwm.h"
#include "bsp_stwlc_driver.h"
#include "global_signals.h"
#include "project_settings.h"
#include "qpc.h"

#if defined(USE_QPC)
Q_DEFINE_THIS_FILE // define the name of this file for assertions
#endif

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/

#define MAX_CHARGE_CURRENT         950
#define ON_CHARGE_CURRENT          475
#define TRICKLE_CHARGE_CURRENT     75
#define MAX_BATTERY_TEMPERATURE_mC 4500

#define MIN_BATTERY_SOC            20
#define MIN_BATTERY_VOLTAGE_V      14
#define MAX_BATTERY_VOLTAGE_V      18

#define VOLTAGE_RATIO              1000U / 1525U
#define CURRENT_RATIO              12500U/* (1/80mV)*1000 fattore di conversione sensore in mA */
#define OFFSET_CURR_SENSOR         2U /*La corrente iniziale non è 0, ma 9mA.. ne tengo conto*/

#define SEPIC_EFF                  26U /* Efficienza del convertitore SEPIC, da misurare sperimentalmente */
#define MAX_WLC_POWER_W       15U  /* Potenza massima erogabile dal caricatore wireless, da misurare sperimentalmente */
#define END_CHARGE_CURRENT_MA 150U /* Corrente di fine carica, da misurare sperimentalmente */
#define MAX_SEPIC_POWER_W     11U  /* Potenza massima erogabile dal convertitore SEPIC, da misurare sperimentalmente */
#define VCC_HALF_MV           1650U//1636U
#define VCC_MV                3270U
#define EEPROM_SAVE_TIME_MS   60 * 1000 * 5
#define SECOND_N_TICKS        1000U //770U//385U
#define N_MOVING_AVERAGE_SAMPLES 5U

  /*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

  /*****************************************************************************
* Module Typedefs
******************************************************************************/

  typedef struct {
  uint16_t soc;
  uint16_t vbat_raw;
  uint16_t ibat_raw;
  uint16_t tbat_raw;
  uint16_t ibat_adc;
  uint32_t vbat_adc;
  uint16_t ibat_history[N_MOVING_AVERAGE_SAMPLES];
  uint16_t vbat_history[N_MOVING_AVERAGE_SAMPLES];
  uint16_t tbat_history[N_MOVING_AVERAGE_SAMPLES];
  uint16_t ibat_mV_history[N_MOVING_AVERAGE_SAMPLES];
  uint8_t filter_index;
  uint8_t sample_count;
  uint32_t ibat_sum;
  uint32_t vbat_sum;
  uint32_t tbat_sum;
  uint32_t ibat_mV_sum;
  uint16_t ibat_mm;
  uint16_t vbat_mm;
  uint16_t tbat_mm;
  uint16_t half_vcc_curr_mm;
} batteryInfo_t;

// Active Object
typedef struct {
  QActive super;
  QTimeEvt timerEvt;
  batteryInfo_t battInfo;
  bool LedIsOn;
  bool moving_average_initialized;
  uint16_t termination_voltage_mV;
  uint16_t half_vcc_curr;
  uint32_t cnt;
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

volatile uint16_t debug_current = 300U;

enum {
  CHARGER_CC_STATE_CHARGING = 0,
  CHARGER_CV_STATE_CHARGING,
  CHARGER_STATE_CC_COMPLETED,
  CHARGER_STATE_CV_COMPLETED,
  CHARGER_STATE_CURR_ERROR,
  CHARGER_STATE_TEMP_ERROR,
  CHARGER_STATE_VOLT_ERROR
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
static QState battery_manager_end_charge_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_on_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_high_level_batt_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_low_level_batt_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_alarm_state(BatteryManager_t* const me, QEvt const* const e);
//static QState battery_manager_debug_state(BatteryManager_t* const me, QEvt const* const e);

static void batteryInfo_update_moving_average(batteryInfo_t* const battInfo, BatteryManager_t* const me);
static QState charger_cc_manager(BatteryManager_t* const me, batteryInfo_t battInfo, uint32_t max_charge_curr_mA);
static QState charger_cv_manager(BatteryManager_t* const me, batteryInfo_t battInfo, uint32_t max_charge_curr_mA);
static uint32_t current_to_pwm(uint32_t curr_mA);
static void batteryInfo_process_adc(BatteryManager_t *me, AdcInfoEvt const *evt);

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
      static const QEvt evt = QEVT_INITIALIZER(INITIALIZE_SIG); // lo farà l'EEPROM quando è ready
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
      //bsp_i2c_init(&me->super);
      status = Q_TRAN(&battery_manager_active_state);
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

// static QState
// battery_manager_debug_state(BatteryManager_t* const me, QEvt const* const e) {
//   QState status;
//   switch (e->sig) {

//     case Q_ENTRY_SIG: {
//       bsp_digital_output_set(IO_BAT_SW_EN, IO_ON);
//       //bsp_pwm_set_duty(50U);
//       // bsp_digital_output_set(IO_BAT_SW_EN, IO_ON);
//       // bsp_single_led_set(LED_GREEN, LED_ON);
//       // QTimeEvt_armX(&me->timerEvt, SECOND_N_TICKS*60, 0);
//       status = Q_HANDLED();
//       break;
//     }

//     case TIMEOUT_SIG: {
//       bsp_single_led_set(LED_BLUE, LED_ON);
//       //QTimeEvt_armX(&me->timerEvt, 1000, 0);
//       status = Q_HANDLED();
//       break;
//     }

//     case Q_EXIT_SIG: {
//       status = Q_HANDLED();
//       break;
//     }

//     default: {
//       status = Q_SUPER(&QHsm_top);
//       break;
//     }
//   }
//   return status;
// }

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

    case Q_INIT_SIG: {
      status = Q_TRAN(&battery_manager_startup_state);
      break;
    }

    case ALARM_SIG: {
      status = Q_TRAN(&battery_manager_alarm_state);
      break;
    }

    case ADC_BATTERY_INFO_SAMPLE_SIG: {    
      batteryInfo_process_adc(me, Q_EVT_CAST(AdcInfoEvt));
      static QEvt const evt = QEVT_INITIALIZER(ADC_DATA_READY_SIG);
      QACTIVE_POST(AO_BatteryManager, &evt, 0U);
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
      me->moving_average_initialized = false;
      me->half_vcc_curr = VCC_HALF_MV;
      me->battInfo.filter_index = 0;
      me->battInfo.sample_count = 0;
      me->battInfo.ibat_sum = 0;
      me->battInfo.vbat_sum = 0;
      me->battInfo.tbat_sum = 0;
      me->battInfo.ibat_mV_sum = 0;
      for (uint8_t i = 0; i < N_MOVING_AVERAGE_SAMPLES; i++) {
        me->battInfo.ibat_history[i] = 0;
        me->battInfo.vbat_history[i] = 0;
        me->battInfo.tbat_history[i] = 0;
        me->battInfo.ibat_mV_history[i] = 0;
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
          __NOP();
        }
      } else if (Q_EVT_CAST(keypad_event_t)->btn == BSP_KEYPAD_ON_OFF_BTN) {
        if (Q_EVT_CAST(keypad_event_t)->event == BSP_BUTTON_ONPRESSED_EVENT) {
          bsp_digital_output_set(IO_BAT_SW_EN, IO_ON);
          status = Q_TRAN(&battery_manager_on_state);
          break;
        } 
      }     
      status = Q_HANDLED();
      break;
    }

    case ADC_DATA_READY_SIG: {
      if(me->moving_average_initialized) {
        me->half_vcc_curr = me->battInfo.ibat_adc - OFFSET_CURR_SENSOR;
        status = Q_TRAN(&battery_manager_soft_start_state);
        break;
      }
      status = Q_HANDLED(); 
      break;
    }

    case DELAY_VBAT_TO_VCH_SIG: {
      /* devo togliere e rimettere l'iscrizione all'evento ADC altrimenti la media mobile si caricherebbe
      con valori di corrente ancora non nulli in quanto IO_BAT_SW_EN deve rimane attivo qualche secondo per non far resettare il WLC */
      QTimeEvt_armX(&me->timerEvt, 6000U, 0);
      bsp_color_rgb_set(OFF);
      status = Q_HANDLED();
      break;
    }

    case TIMEOUT_SIG: {
      QActive_subscribe(&me->super, ADC_BATTERY_INFO_SAMPLE_SIG);
      bsp_digital_output_set(IO_BAT_SW_EN, IO_OFF);
      status = Q_HANDLED();
      break;
    }

    case Q_EXIT_SIG: {
      //QTimeEvt_disarm(&me->timerEvt);
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

// static QState
// battery_manager_vaux_state(BatteryManager_t* const me, QEvt const* const e) {
//   QState status;
//   switch (e->sig) {

//     case Q_ENTRY_SIG: {
//       bsp_single_led_set(LED_BLUE, LED_OFF);
//       bsp_single_led_set(LED_GREEN, LED_ON);
//       bsp_single_led_set(LED_RED, LED_ON);
//       bsp_digital_output_set(IO_BAT_SW_EN, IO_OFF);
//       bsp_digital_output_set(IO_V_AUX_EN, IO_ON);
//       QTimeEvt_armX(&me->timerEvt, 3 * SECOND_N_TICKS, 0);
//       status = Q_HANDLED();
//       break;
//     }

//     case TIMEOUT_SIG: {
//       status = Q_TRAN(&battery_manager_soft_start_state);
//       break;
//     }

//     case Q_EXIT_SIG: {
//       QTimeEvt_disarm(&me->timerEvt);
//       status = Q_HANDLED();
//       break;
//     }

//     default: {
//       status = Q_SUPER(&battery_manager_active_state);
//       break;
//     }
//   }
//   return status;
// }

static QState
battery_manager_soft_start_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  static uint16_t ss_tick = 100;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      QActive_unsubscribe(&me->super, ADC_BATTERY_INFO_SAMPLE_SIG);
      bsp_digital_output_set(IO_BAT_SW_EN, IO_OFF);
      bsp_digital_output_set(IO_V_AUX_EN, IO_ON);
      bsp_color_rgb_set(YELLOW);
      /* soft start wlc.. per 3 secondi non accendo il SEPIC, ma solo lo schermo */
      QTimeEvt_armX(&me->timerEvt, 3 * SECOND_N_TICKS, 0);
      status = Q_HANDLED();
      break;
    }

    case ADC_DATA_READY_SIG: {
      /* Verifica se le condizioni di soft-start sono soddisfatte */
      bool vbat_valid = me->battInfo.vbat_mm > 1100U;  /* > 11V */
      bool tbat_valid = me->battInfo.tbat_mm < MAX_BATTERY_TEMPERATURE_mC;

      if (!vbat_valid || !tbat_valid) {
        /* Tensione o temperatura fuori range → allarme */
        status = Q_TRAN(&battery_manager_alarm_state);
      } else {
        /* Incrementa gradualmente il duty cycle del PWM durante il soft-start */
        bsp_pwm_set_duty(current_to_pwm(ss_tick));
        ss_tick += 100U;

        if (ss_tick <= 500U) {
          /* Rampa di soft-start ancora in corso */
          status = Q_HANDLED();
        } else {
          /* Rampa completata: verifica se il SEPIC si è attivato */
          uint16_t current_threshold_mA = 100U;  /* Corrente minima per attivazione SEPIC 150 */

          if (me->battInfo.ibat_mm < current_threshold_mA) {
            /* Corrente insufficiente → batteria è carica, SEPIC non attivo */
            status = Q_TRAN(&battery_manager_end_charge_state);
          } else {
            /* Corrente sufficiente → SEPIC attivo, inizio carica CC */
            status = Q_TRAN(&battery_manager_on_cc_charge_state);
          }
        }
      }
      break;
    }

    case TIMEOUT_SIG: {
      bsp_digital_output_set(IO_CH_PWM_SYNC, IO_ON);
      QActive_subscribe(&me->super, ADC_BATTERY_INFO_SAMPLE_SIG);
      status = Q_HANDLED();
      break;
    }

    case Q_EXIT_SIG: {
      ss_tick = 100;
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
  static uint32_t cc_tick = 0;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      cc_tick = 0;
      bsp_color_rgb_set(GREEN);
      QTimeEvt_armX(&me->timerEvt, SECOND_N_TICKS / 2, 0);
      me->LedIsOn = true;
      status = Q_HANDLED();
      break;
    }

    case ADC_DATA_READY_SIG: {
      status = charger_cc_manager(me, me->battInfo, 500U);
      break;
    }

    case TIMEOUT_SIG: {
      // EepromEvt* evt = Q_NEW(EepromEvt, EEPROM_WRITE_SIG);
      // /*calculate the mAh per 5 minutes */
      // evt->data = ON_CHARGE_CURRENT * (5 / 60);
      // QF_PUBLISH(&evt->super, me);
      //status = Q_HANDLED();
      //break;
      QTimeEvt_armX(&me->timerEvt, SECOND_N_TICKS / 2, 0);
      if (me->LedIsOn) {
        bsp_color_rgb_set(OFF);
        me->LedIsOn = false;
      } else {
        bsp_color_rgb_set(GREEN);
        me->LedIsOn = true;
      }
      if (++cc_tick >= 32400) {
        /* 4 ore e mezza in CC, se non è ancora arrivato alla tensione di fine carica, passo comunque alla fase di CV */
        status = Q_TRAN(&battery_manager_end_charge_state);
        break;
      }
      status = Q_HANDLED();
      break;
    }

    case Q_EXIT_SIG: {
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
battery_manager_on_cv_charge_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  static uint32_t cv_tick = 0;

  switch (e->sig) {

    case Q_ENTRY_SIG: {
      cv_tick = 0;
      bsp_color_rgb_set(YELLOW);
      QTimeEvt_armX(&me->timerEvt, SECOND_N_TICKS / 2, 0);
      me->LedIsOn = true;
      status = Q_HANDLED();
      break;
    }

    case ADC_DATA_READY_SIG: {
      status = charger_cv_manager(me, me->battInfo, 500U);
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
      if (++cv_tick >= 240) {
        /* resto al massimo 15 min in CV */
        status = Q_TRAN(&battery_manager_end_charge_state);
        break;
      }
      status = Q_HANDLED();
      break;
    }

    case Q_EXIT_SIG: {
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
battery_manager_end_charge_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      bsp_digital_output_set(IO_CH_PWM_SYNC, IO_OFF);
      bsp_color_rgb_set(GREEN);
      status = Q_HANDLED();
      break;
    }

    case Q_EXIT_SIG: {
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
      bsp_digital_output_set(IO_V_AUX_EN, IO_OFF);
      bsp_digital_output_set(IO_BAT_SW_EN, IO_ON);
      status = Q_HANDLED();
      break;
    }

    case Q_INIT_SIG: {
      status = Q_TRAN(&battery_manager_high_level_batt_state);
      break;
    }

    case BUTTON_PRESSED_SIG: {
      if (Q_EVT_CAST(keypad_event_t)->btn == BSP_KEYPAD_CHARGE_BTN) {
        if (Q_EVT_CAST(keypad_event_t)->event == BSP_BUTTON_ONPRESSED_EVENT) {
          QActive_unsubscribe(&me->super, ADC_BATTERY_INFO_SAMPLE_SIG);
          static QEvt const evt = QEVT_INITIALIZER(DELAY_VBAT_TO_VCH_SIG);
          QACTIVE_POST(AO_BatteryManager, &evt, 0U);
          status = Q_TRAN(&battery_manager_startup_state);
          break;
        }
      } else if (Q_EVT_CAST(keypad_event_t)->btn == BSP_KEYPAD_ON_OFF_BTN) {
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
      bsp_color_rgb_set(BLUE);
      status = Q_HANDLED();
      break;
    }

    case ADC_DATA_READY_SIG: {
      /* Determine required voltage threshold based on current draw */
      uint16_t required_vbat = (me->battInfo.ibat_mm < 250U) ? (14U * 100U) : (13 * 100U);//14

      if (me->battInfo.vbat_mm > required_vbat && me->battInfo.tbat_mm < (MAX_BATTERY_TEMPERATURE_mC)) {
        status = Q_HANDLED();
      } else {
        status = Q_TRAN(&battery_manager_low_level_batt_state);
      }
      break;
    }

    case Q_EXIT_SIG: {
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
      bsp_color_rgb_set(WHITE);
      QTimeEvt_armX(&me->timerEvt, SECOND_N_TICKS * 60 * 5, 0);
      status = Q_HANDLED();
      break;
    }

    case ADC_DATA_READY_SIG: {
      if (me->battInfo.vbat_mm < (13U * 100U) || me->battInfo.tbat_mm > (MAX_BATTERY_TEMPERATURE_mC)) {
        status = Q_TRAN(&battery_manager_alarm_state);
        break;      
      }
      status = Q_HANDLED(); 
      break;
    }

    case TIMEOUT_SIG: {
      /* se dopo 5 minuti il livello di batteria è salito nuovamente sopra i 14V lo mando nuovamente in high level */
      if (me->battInfo.vbat_mm >= (14U * 100U) && me->battInfo.tbat_mm < (MAX_BATTERY_TEMPERATURE_mC)) {
        status = Q_TRAN(&battery_manager_high_level_batt_state);
      } else {
        status = Q_TRAN(&battery_manager_alarm_state);
      }
      break;
    }

    case Q_EXIT_SIG: {
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
battery_manager_alarm_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      bsp_digital_output_set(IO_CH_PWM_SYNC, IO_OFF);
      bsp_digital_output_set(IO_V_AUX_EN, IO_OFF);
      bsp_color_rgb_set(RED);
      QTimeEvt_armX(&me->timerEvt, SECOND_N_TICKS * 5, 0);
      status = Q_HANDLED();
      break;
    }

    case TIMEOUT_SIG: {
      /* turn-off board */
      bsp_digital_output_set(IO_BAT_SW_EN, IO_OFF);
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
charger_cc_manager(BatteryManager_t* const me, batteryInfo_t battInfo, uint32_t max_charge_curr_mA) {
  int idx_tbat, idx_ibat;
  static uint32_t charge_curr_mA;

  /* Controllo tensione minima  e massima batteria */
  if (me->battInfo.vbat_mm < (8U * 100U) || me->battInfo.vbat_mm > (18U * 100U)) {
    return Q_TRAN(&battery_manager_alarm_state);
  }
  /* Controllo corrente minima e massima batteria */
  if(me->battInfo.ibat_mm < 150U || me->battInfo.ibat_mm > 750U) {
    return Q_TRAN(&battery_manager_alarm_state);
  }
    /* Temperatura fuori range → allarme */
  if (battInfo.tbat_mm >= 4500U) {
    return Q_TRAN(&battery_manager_alarm_state);
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
    return Q_TRAN(&battery_manager_alarm_state);
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
    return Q_TRAN(&battery_manager_alarm_state);
  }

  uint16_t termination_mV = termination_voltage_charge_mV[idx_tbat][idx_ibat];
  me->termination_voltage_mV = termination_mV;
  uint16_t vbat_mm_toll = battInfo.vbat_mm * 101
                              / 100; // tensione di fine carica per cella, da visualizzare all'utente
  /* Stato CC */
  if (vbat_mm_toll <= termination_mV) {
    return Q_HANDLED();
  }
  /* Stato CV */
  else {
    return Q_TRAN(&battery_manager_on_cv_charge_state);
  }
}

static QState
charger_cv_manager(BatteryManager_t* const me, batteryInfo_t battInfo, uint32_t max_charge_curr_mA) {
  /* Controllo tensione minima  e massima batteria */
  if (me->battInfo.vbat_mm < (8U * 100U) || me->battInfo.vbat_mm > (18U * 100U)) {
    return Q_TRAN(&battery_manager_alarm_state);
  }
  /* Controllo corrente minima e massima batteria */
  if(me->battInfo.ibat_mm < 100U || me->battInfo.ibat_mm > 750U) {
    return Q_TRAN(&battery_manager_alarm_state);
  }
    /* Temperatura fuori range → allarme */
  if (battInfo.tbat_mm >= 4500U) {
    return Q_TRAN(&battery_manager_alarm_state);
  }
  /* Imposta la corrente di carica */
  bsp_pwm_set_duty(current_to_pwm(max_charge_curr_mA));
  /* fermo presto la carica in CV per preservare la batteria*/
  if (battInfo.ibat_mm >= 400U) {
    return Q_HANDLED();
  } else {
    return Q_TRAN(&battery_manager_end_charge_state);
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

  battInfo->ibat_mV_sum -= battInfo->ibat_mV_history[battInfo->filter_index];
  battInfo->ibat_mV_history[battInfo->filter_index] = battInfo->ibat_adc;
  battInfo->ibat_mV_sum += battInfo->ibat_adc;

  if (battInfo->sample_count < N_MOVING_AVERAGE_SAMPLES) {
    battInfo->sample_count += 1U;
  }

  battInfo->filter_index = (battInfo->filter_index + 1U) % N_MOVING_AVERAGE_SAMPLES;

  if (battInfo->sample_count < N_MOVING_AVERAGE_SAMPLES) {
    battInfo->ibat_mm = battInfo->ibat_raw;
    battInfo->vbat_mm = battInfo->vbat_raw;
    battInfo->tbat_mm = battInfo->tbat_raw;
    battInfo->half_vcc_curr_mm = battInfo->ibat_adc;
  } else {
    if(me->moving_average_initialized == false) {
      me->moving_average_initialized = true;
    }
    battInfo->ibat_mm = (uint16_t)((battInfo->ibat_sum + (N_MOVING_AVERAGE_SAMPLES - 1U)) / N_MOVING_AVERAGE_SAMPLES);
    battInfo->vbat_mm = (uint16_t)((battInfo->vbat_sum + (N_MOVING_AVERAGE_SAMPLES - 1U)) / N_MOVING_AVERAGE_SAMPLES);
    battInfo->half_vcc_curr_mm = (uint16_t)((battInfo->ibat_mV_sum + (N_MOVING_AVERAGE_SAMPLES - 1U)) / N_MOVING_AVERAGE_SAMPLES);
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

  /* ViSET = (I * R3 + IFBVOS) * (R5 / R4) */
  float Viset = (Ichg * 0.14f + 0.0f) * 18.0f;

  /* Converto ViSET nel duty-cycle PWM */
  float pwm = 100.0f * (Viset / 3.0f);

  if (pwm < 0.0f) {
    pwm = 0.0f;
  }
  if (pwm > 100.0f) {
    pwm = 100.0f;
  }

  uint32_t pwm_value = (uint32_t)(pwm + 0.5f); // Arrotonda al valore intero più vicino
  return pwm_value;
}

static void batteryInfo_process_adc(BatteryManager_t *me, AdcInfoEvt const *evt)
{
    uint32_t numerator_curr;
    uint32_t denominator_curr;

    /* SOC e valori ADC grezzi */
    me->battInfo.soc       = evt->soc;
    me->battInfo.ibat_adc  = evt->ibat;
    me->battInfo.tbat_raw  = evt->tbat;

    /* Calcolo corrente batteria */
    numerator_curr =
        (((me->half_vcc_curr > evt->ibat)
            ? (me->half_vcc_curr - evt->ibat)
            : (evt->ibat - me->half_vcc_curr))
         * CURRENT_RATIO);

    denominator_curr = me->half_vcc_curr * 2U;

    /* Arrotondamento per eccesso */
    me->battInfo.ibat_raw =
        (uint16_t)((numerator_curr + (denominator_curr - 1U)) / denominator_curr);

    /* Tensione batteria */
    me->battInfo.vbat_raw = evt->vbat * VOLTAGE_RATIO;
    me->battInfo.vbat_adc = (evt->vbat * 65535U) / 3330U;

    /* Media mobile */
    batteryInfo_update_moving_average(&me->battInfo, me);
}