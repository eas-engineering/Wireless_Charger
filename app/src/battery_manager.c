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

#define MAX_CHARGE_CURRENT        950
#define ON_CHARGE_CURRENT         475
#define TRICKLE_CHARGE_CURRENT    75
#define MAX_BATTERY_TEMPERATURE_C 45

#define MIN_BATTERY_SOC           20
#define MIN_BATTERY_VOLTAGE_V     13
#define MAX_BATTERY_VOLTAGE_V     18

#define VOLTAGE_RATIO             1000U / 1525U
#define CURRENT_RATIO             12500U

#define SEPIC_EFF                 26U /* Efficienza del convertitore SEPIC, da misurare sperimentalmente */
#define MAX_WLC_POWER_W           15U  /* Potenza massima erogabile dal caricatore wireless, da misurare sperimentalmente */
#define END_CHARGE_CURRENT_MA     150U /* Corrente di fine carica, da misurare sperimentalmente */
#define MAX_SEPIC_POWER_W         11U  /* Potenza massima erogabile dal convertitore SEPIC, da misurare sperimentalmente */
#define VCC_HALF_MV               1636U
#define VCC_MV                    3272U
#define EEPROM_SAVE_TIME_MS       60 * 1000 * 5

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
  uint16_t ibat_history[5];
  uint16_t vbat_history[5];
  uint16_t tbat_history[5];
  uint8_t filter_index;
  uint8_t sample_count;
  uint32_t ibat_sum;
  uint32_t vbat_sum;
  uint32_t tbat_sum;
  uint16_t ibat_mm;
  uint16_t vbat_mm;
  uint16_t tbat_mm;
} batteryInfo_t;

// Active Object
typedef struct {
  QActive super;
  QTimeEvt timerEvt;
  batteryInfo_t battInfo;
  bool LedIsOn;
  uint16_t termination_voltage_mV;
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
static QState battery_manager_alarm_state(BatteryManager_t* const me, QEvt const* const e);

static QState battery_manager_debug_state(BatteryManager_t* const me, QEvt const* const e);

static void batteryInfo_update_moving_average(batteryInfo_t* const battInfo);
//static uint8_t charger_cc_manager(batteryInfo_t battInfo, uint32_t max_charge_curr_mA);
static QState charger_cc_manager(BatteryManager_t* const me, batteryInfo_t battInfo, uint32_t max_charge_curr_mA);
static QState charger_cv_manager(BatteryManager_t* const me, batteryInfo_t battInfo, uint32_t max_charge_curr_mA);
static uint32_t current_to_pwm(uint32_t curr_mA);

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

static BatteryManager_t BatteryManager;
QActive* const AO_BatteryManager = &BatteryManager.super;
static uint32_t ss_tick = 100;
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
  QActive_subscribe(&me->super, ADC_BATTERY_INFO_SAMPLE_SIG);
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

      static const QEvt evt = QEVT_INITIALIZER(INITIALIZE_SIG); // lo farà l'EEPROM quando è ready
      QACTIVE_POST(AO_BatteryManager, &evt, 0U);
      status = Q_HANDLED();
      break;
    }

    case INITIALIZE_SIG: {
      bsp_adc_init();
      bsp_pwm_init();
      bsp_digital_output_init();
      bsp_led_init();
      keypad_init();
      bsp_i2c_init(&me->super);

      /* Inizializza filtri media mobile */
      me->battInfo.filter_index = 0;
      me->battInfo.sample_count = 0;
      me->battInfo.ibat_sum = 0;
      me->battInfo.vbat_sum = 0;
      me->battInfo.tbat_sum = 0;
      for (int i = 0; i < 5; i++) {
        me->battInfo.ibat_history[i] = 0;
        me->battInfo.vbat_history[i] = 0;
        me->battInfo.tbat_history[i] = 0;
      }
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
battery_manager_debug_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      bsp_pwm_init();
      bsp_pwm_set_duty(current_to_pwm(debug_current));
      bsp_digital_output_set(IO_CH_PWM_SYNC, IO_ON);
      QTimeEvt_armX(&me->timerEvt, 1000, 0);
      status = Q_HANDLED();
      break;
    }

    case TIMEOUT_SIG: {
      bsp_pwm_set_duty(current_to_pwm(debug_current));
      QTimeEvt_armX(&me->timerEvt, 1000, 0);
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
      status = Q_HANDLED();
      break;
    }

    case BUTTON_PRESSED_SIG: {
      if (Q_EVT_CAST(keypad_event_t)->btn == BSP_KEYPAD_CHARGE_BTN) {
        if (Q_EVT_CAST(keypad_event_t)->event == BSP_BUTTON_ONPRESSED_EVENT) {
          status = Q_TRAN(&battery_manager_soft_start_state);
        }
      } else if (Q_EVT_CAST(keypad_event_t)->btn == BSP_KEYPAD_ON_OFF_BTN) {
        if (Q_EVT_CAST(keypad_event_t)->event == BSP_BUTTON_ONPRESSED_EVENT) {
          status = Q_TRAN(&battery_manager_on_state);
        }
      } else {
        status = Q_HANDLED();
      }
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
      status = Q_TRAN(&battery_manager_alarm_state);
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
battery_manager_soft_start_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      bsp_led_set(LED_BLUE, LED_OFF);
      bsp_led_set(LED_GREEN, LED_ON);
      bsp_digital_output_set(IO_V_AUX_EN, IO_ON);
      bsp_digital_output_set(IO_CH_PWM_SYNC, IO_ON);
      status = Q_HANDLED();
      break;
    }

    case ADC_BATTERY_INFO_SAMPLE_SIG: {
      me->battInfo.soc = Q_EVT_CAST(AdcInfoEvt)->soc;
      me->battInfo.ibat_raw = (uint16_t)((((VCC_HALF_MV > Q_EVT_CAST(AdcInfoEvt)->ibat)
                                             ? (VCC_HALF_MV - Q_EVT_CAST(AdcInfoEvt)->ibat)
                                             : (Q_EVT_CAST(AdcInfoEvt)->ibat - VCC_HALF_MV))
                                          * CURRENT_RATIO)
                                         / VCC_MV);
      me->battInfo.vbat_raw = Q_EVT_CAST(AdcInfoEvt)->vbat * VOLTAGE_RATIO;
      me->battInfo.tbat_raw = Q_EVT_CAST(AdcInfoEvt)->tbat;
      batteryInfo_update_moving_average(&me->battInfo);
      if (me->battInfo.vbat_mm > MIN_BATTERY_VOLTAGE_V * 100U
          && me->battInfo.tbat_mm < MAX_BATTERY_TEMPERATURE_C * 100U) {
        //QTimeEvt_armX(&me->timerEvt, 0U, 0);
        //status = Q_HANDLED();
        bsp_pwm_set_duty(current_to_pwm(ss_tick));
        ss_tick += 100U;
        if (ss_tick > 500U) {
          status = Q_TRAN(&battery_manager_on_cc_charge_state);
        }
      } else {
        status = Q_TRAN(&battery_manager_alarm_state);
      }
      break;
    }

    case TIMEOUT_SIG: {
      // bsp_pwm_set_duty(current_to_pwm(ss_tick));
      // ss_tick += 100U;
      // if (ss_tick > 500U) {
      //   status = Q_TRAN(&battery_manager_on_cc_charge_state);
      // }
      status = Q_HANDLED();
      break;
    }

    case Q_EXIT_SIG: {
      QTimeEvt_disarm(&me->timerEvt);
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
 * @details This state is responsible for managing the battery charging process.
 *        It will transition to the trickle state upon receiving the ALARM_SIG event.
 *        It will transition back to the active state upon receiving the Q_EXIT_SIG event.
 */
static QState
battery_manager_on_cc_charge_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      bsp_digital_output_set(IO_BAT_SW_EN, IO_OFF);
      // bsp_digital_output_set(IO_BAT_SW_EN, IO_OFF);
      // bsp_led_set(LED_BLUE, LED_OFF);
      // bsp_led_set(LED_GREEN, LED_ON);
      // bsp_digital_output_set(IO_V_AUX_EN, IO_ON);
      // bsp_digital_output_set(IO_CH_PWM_SYNC, IO_ON);
      //bsp_pwm_init();
      //bsp_pwm_set_duty(current_to_pwm(300U));
      //bsp_adc_init();
      //QTimeEvt_armX(&me->timerEvt, 2000, 0);
      status = Q_HANDLED();
      break;
    }

    case ADC_BATTERY_INFO_SAMPLE_SIG: {
      me->battInfo.soc = Q_EVT_CAST(AdcInfoEvt)->soc;
      me->battInfo.ibat_raw = (uint16_t)((((VCC_HALF_MV > Q_EVT_CAST(AdcInfoEvt)->ibat)
                                             ? (VCC_HALF_MV - Q_EVT_CAST(AdcInfoEvt)->ibat)
                                             : (Q_EVT_CAST(AdcInfoEvt)->ibat - VCC_HALF_MV))
                                          * CURRENT_RATIO)
                                         / VCC_MV);
      me->battInfo.vbat_raw = Q_EVT_CAST(AdcInfoEvt)->vbat * VOLTAGE_RATIO;
      me->battInfo.tbat_raw = Q_EVT_CAST(AdcInfoEvt)->tbat;
      batteryInfo_update_moving_average(&me->battInfo);
      status = charger_cc_manager(me, me->battInfo, 650U);
      //uint8_t charger_state = charger_cc_manager(me->battInfo, 700U);
      // switch (charger_state)
      // {
      //   case CHARGER_CC_STATE_CHARGING:
      //     status = Q_HANDLED();
      //     break;
      //   case CHARGER_CV_STATE_CHARGING:
      //     status = Q_HANDLED();
      //     break;
      //   case CHARGER_STATE_CV_COMPLETED:
      //     status = Q_TRAN(&battery_manager_end_charge_state);
      //     break;
      //   case CHARGER_STATE_CURR_ERROR:
      //     status = Q_TRAN(&battery_manager_alarm_state);
      //     break;
      //   case CHARGER_STATE_TEMP_ERROR:
      //     status = Q_TRAN(&battery_manager_alarm_state);
      //     break;
      //   case CHARGER_STATE_VOLT_ERROR:
      //     status = Q_TRAN(&battery_manager_alarm_state);
      //     break;
      //   default:
      //     status = Q_TRAN(&battery_manager_alarm_state);
      //     break;
      // }
      break;
    }

    case TIMEOUT_SIG: {
      // EepromEvt* evt = Q_NEW(EepromEvt, EEPROM_WRITE_SIG);
      // /*calculate the mAh per 5 minutes */
      // evt->data = ON_CHARGE_CURRENT * (5 / 60);
      // QF_PUBLISH(&evt->super, me);
      //status = Q_HANDLED();
      //break;
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
      QTimeEvt_armX(&me->timerEvt, 10000, 0);
      status = Q_HANDLED();
      break;
    }

    case ADC_BATTERY_INFO_SAMPLE_SIG: {
      me->battInfo.soc = Q_EVT_CAST(AdcInfoEvt)->soc;
      me->battInfo.ibat_raw = (uint16_t)((((VCC_HALF_MV > Q_EVT_CAST(AdcInfoEvt)->ibat)
                                             ? (VCC_HALF_MV - Q_EVT_CAST(AdcInfoEvt)->ibat)
                                             : (Q_EVT_CAST(AdcInfoEvt)->ibat - VCC_HALF_MV))
                                          * CURRENT_RATIO)
                                         / VCC_MV);
      me->battInfo.vbat_raw = Q_EVT_CAST(AdcInfoEvt)->vbat * VOLTAGE_RATIO;
      me->battInfo.tbat_raw = Q_EVT_CAST(AdcInfoEvt)->tbat;
      batteryInfo_update_moving_average(&me->battInfo);
      status = charger_cv_manager(me, me->battInfo, 600U);
      break;
    }

    case TIMEOUT_SIG: {
      if(++cv_tick > 60*6) { // se sono passati più di 60 minuti in CV, esco comunque dallo stato di carica
        status = Q_TRAN(&battery_manager_end_charge_state);
        cv_tick = 0;
      } else {
        QTimeEvt_armX(&me->timerEvt, 10000, 0);
        status = Q_HANDLED();
      }
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
      bsp_pwm_set_duty(current_to_pwm(0U));
      bsp_pwm_deinit();
      QTimeEvt_armX(&me->timerEvt, 500, 0);
      me->LedIsOn = true;
      status = Q_HANDLED();
      break;
    }

    case TIMEOUT_SIG: {
      if (me->LedIsOn) {
        bsp_led_set(LED_GREEN, LED_OFF);
        me->LedIsOn = false;
      } else {
        bsp_led_set(LED_GREEN, LED_ON);
        me->LedIsOn = true;
      }

      QTimeEvt_armX(&me->timerEvt, 500, 0);
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
      bsp_digital_output_set(IO_BAT_SW_EN, IO_ON);
      bsp_led_set(LED_BLUE, LED_ON);
      status = Q_HANDLED();
      break;
    }

    case ADC_BATTERY_INFO_SAMPLE_SIG: {
      me->battInfo.soc = Q_EVT_CAST(AdcInfoEvt)->soc;
      me->battInfo.ibat_raw = (uint16_t)((((VCC_HALF_MV > Q_EVT_CAST(AdcInfoEvt)->ibat)
                                             ? (VCC_HALF_MV - Q_EVT_CAST(AdcInfoEvt)->ibat)
                                             : (Q_EVT_CAST(AdcInfoEvt)->ibat - VCC_HALF_MV))
                                          * CURRENT_RATIO)
                                         / VCC_MV);
      me->battInfo.vbat_raw = Q_EVT_CAST(AdcInfoEvt)->vbat * VOLTAGE_RATIO;
      me->battInfo.tbat_raw = Q_EVT_CAST(AdcInfoEvt)->tbat;
      batteryInfo_update_moving_average(&me->battInfo);

      /*TODO*/
      if (me->battInfo.vbat_mm > (MIN_BATTERY_VOLTAGE_V * 100U)
          && me->battInfo.tbat_mm < (MAX_BATTERY_TEMPERATURE_C * 100U)) {
        status = Q_HANDLED();
      } else {
        static QEvt const evt = QEVT_INITIALIZER(OFF_SIG);
        QACTIVE_POST(AO_BatteryManager, &evt, 0U);
      }
      status = Q_HANDLED();
      break;
    }

    case BUTTON_PRESSED_SIG: {
      if (Q_EVT_CAST(keypad_event_t)->btn == BSP_KEYPAD_CHARGE_BTN) {
        if (Q_EVT_CAST(keypad_event_t)->event == BSP_BUTTON_ONPRESSED_EVENT) {
          status = Q_TRAN(&battery_manager_soft_start_state);
        }
      } else if (Q_EVT_CAST(keypad_event_t)->btn == BSP_KEYPAD_ON_OFF_BTN) {
        if (Q_EVT_CAST(keypad_event_t)->event == BSP_BUTTON_ONPRESSED_EVENT) {
          static QEvt const evt = QEVT_INITIALIZER(OFF_SIG);
          QACTIVE_POST(AO_BatteryManager, &evt, 0U);
        } else {
          status = Q_HANDLED();
        }
      } else {
        status = Q_HANDLED();
      }
      break;
    }

    case OFF_SIG: {
      bsp_led_set(LED_BLUE, LED_OFF);
      bsp_digital_output_set(IO_BAT_SW_EN, IO_OFF);
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
      bsp_pwm_set_duty(current_to_pwm(0U));
      bsp_pwm_deinit();
      bsp_digital_output_set(IO_V_AUX_EN, IO_OFF);
      bsp_led_set(LED_BLUE, LED_OFF);
      bsp_led_set(LED_GREEN, LED_OFF);
      bsp_led_set(LED_RED, LED_ON);
      QTimeEvt_armX(&me->timerEvt, 5000, 0);
      status = Q_HANDLED();
      break;
    }

    case TIMEOUT_SIG: {
      bsp_digital_output_set(IO_BAT_SW_EN, IO_OFF);
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
 * @brief Controlla se la batteria e' stata completamente caricata
 *
 * La funzione controlla se la batteria e' stata completamente caricata
 * in base alla sua temperatura e corrente di carica.
 *
 * La funzione utilizza le tabelle "temp_ranges" e "current_ranges_mA"
 * per determinare l'indice di temperatura e corrente di carica
 * e successivamente utilizza la tabella "termination_voltage_charge_mV"
 * per determinare il valore di tensione di terminazione della carica.
 *
 * @param battInfo Informazioni sulla batteria
 * @param charge_cur_mA Corrente di carica attuale in mA
 * @return true se la batteria e' stata completamente caricata, false altrimenti
 */
// static uint8_t
// charger_cc_manager(batteryInfo_t battInfo, uint32_t max_charge_curr_mA) {
//   int idx_tbat, idx_ibat;
//   static uint32_t charge_curr_mA;

//   if(battInfo.vbat_mm < (MIN_BATTERY_VOLTAGE_V * 100)) {
//     return CHARGER_STATE_VOLT_ERROR;
//   }

//   /* Corrente massima erogabile dal SEPIC in mA, calcolata come P=V*I */
//   charge_curr_mA = MAX_SEPIC_POWER_W * 100 * 1000 / battInfo.vbat_mm;
//   if(charge_curr_mA > max_charge_curr_mA) {
//     charge_curr_mA = max_charge_curr_mA;
//   }

//   /* Trova indice fascia temperatura */
//   for (idx_tbat = 0; idx_tbat < 5; idx_tbat++) {
//     if( (battInfo.tbat_mm >= temp_ranges[idx_tbat][0]) && (battInfo.tbat_mm < temp_ranges[idx_tbat ][1]) ) {
//       break;
//     }
//   }
//   /* se la temperatura è fuori dalle fasce definite, blocco la carica */
//   if (idx_tbat >= 5) {
//     return CHARGER_STATE_TEMP_ERROR;
//   }

//   /* imposto corrente di uscita */
//   bsp_pwm_set_duty(current_to_pwm(charge_curr_mA));//charge_curr_mA

//   /* Trova indice fascia corrente (It rate) */
//   for (idx_ibat = 0; idx_ibat < 3; idx_ibat++) {
//     if ( (charge_curr_mA >= current_ranges_mA[idx_ibat][0]) && (charge_curr_mA <= current_ranges_mA[idx_ibat][1]) ) {
//       break;
//     }
//   }
//   if (idx_ibat >= 3) {
//     return CHARGER_STATE_CURR_ERROR;
//   }

//   uint16_t termination_mV = termination_voltage_charge_mV[idx_tbat][idx_ibat];

//   /* Controllo terminazione */
//   if (battInfo.vbat_mm <= termination_mV) {
//     return CHARGER_CC_STATE_CHARGING;
//   }
//   else if((battInfo.vbat_mm > termination_mV) && (charge_curr_mA > END_CHARGE_CURRENT_MA)) {
//     return CHARGER_CV_STATE_CHARGING;
//   }
//   return CHARGER_STATE_CV_COMPLETED;
// }
static QState
charger_cc_manager(BatteryManager_t* const me, batteryInfo_t battInfo, uint32_t max_charge_curr_mA) {
  int idx_tbat, idx_ibat;
  static uint32_t charge_curr_mA;

  /* Controllo tensione minima batteria */
  if (battInfo.vbat_mm < (MIN_BATTERY_VOLTAGE_V * 100U)) {
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
  /* Stato CC */
  if (battInfo.vbat_mm <= termination_mV) {
    return Q_HANDLED();
  }
  /* Stato CV */
  else /*if ((battInfo.vbat_mm > termination_mV) && (charge_curr_mA > END_CHARGE_CURRENT_MA))*/ {
    return Q_TRAN(&battery_manager_on_cv_charge_state);
  }

  // /* Fine carica */
  // return Q_TRAN(&battery_manager_on_cv_charge_state);
}

static QState
charger_cv_manager(BatteryManager_t* const me, batteryInfo_t battInfo, uint32_t max_charge_curr_mA) {
  /* Controllo tensione minima batteria */
  if (battInfo.vbat_mm < (MIN_BATTERY_VOLTAGE_V * 100U) || battInfo.vbat_mm > (MAX_BATTERY_VOLTAGE_V * 100U)) {
    return Q_TRAN(&battery_manager_alarm_state);
  }
  /* Temperatura fuori range → allarme */
  if (battInfo.tbat_mm >= MAX_BATTERY_TEMPERATURE_C * 100U) {
    return Q_TRAN(&battery_manager_alarm_state);
  }
  /* Imposta la corrente di carica */
  bsp_pwm_set_duty(current_to_pwm(max_charge_curr_mA));

  uint16_t termination_mA = END_CHARGE_CURRENT_MA;

  if (battInfo.ibat_mm >= termination_mA) {
    return Q_HANDLED();
  } else {
    return Q_TRAN(&battery_manager_end_charge_state);
  }
}

static void
batteryInfo_update_moving_average(batteryInfo_t* const battInfo) {
  battInfo->ibat_sum -= battInfo->ibat_history[battInfo->filter_index];
  battInfo->ibat_history[battInfo->filter_index] = battInfo->ibat_raw;
  battInfo->ibat_sum += battInfo->ibat_raw;

  battInfo->vbat_sum -= battInfo->vbat_history[battInfo->filter_index];
  battInfo->vbat_history[battInfo->filter_index] = battInfo->vbat_raw;
  battInfo->vbat_sum += battInfo->vbat_raw;

  battInfo->tbat_sum -= battInfo->tbat_history[battInfo->filter_index];
  battInfo->tbat_history[battInfo->filter_index] = battInfo->tbat_raw;
  battInfo->tbat_sum += battInfo->tbat_raw;

  if (battInfo->sample_count < 5U) {
    battInfo->sample_count += 1U;
  }

  battInfo->filter_index = (battInfo->filter_index + 1U) % 5U;

  if (battInfo->sample_count < 5U) {
    battInfo->ibat_mm = battInfo->ibat_raw;
    battInfo->vbat_mm = battInfo->vbat_raw;
    battInfo->tbat_mm = battInfo->tbat_raw;
  } else {
    battInfo->ibat_mm = (uint16_t)(battInfo->ibat_sum / 5U);
    battInfo->vbat_mm = (uint16_t)(battInfo->vbat_sum / 5U);
    battInfo->tbat_mm = (uint16_t)(battInfo->tbat_sum / 5U);
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
