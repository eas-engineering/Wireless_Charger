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
#include "bsp_i2c.h"
#include "bsp_pwm.h"
#include "bsp_stwlc_driver.h"
#include "fsl_common.h"
#include "fsl_ctimer.h"
#include "fsl_gpio.h"
#include "fsl_inputmux.h"
#include "fsl_lpadc.h"
#include "fsl_port.h"
#include "global_signals.h"
#include "project_settings.h"
#include "qpc.h"

#if defined(USE_QPC)
Q_DEFINE_THIS_FILE // define the name of this file for assertions
#endif

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/

#define V_AUX_EN_GPIO          GPIO3
#define V_AUX_EN_PORT          PORT3
#define V_AUX_EN_PIN           0U

#define BAT_SW_EN_GPIO         GPIO1
#define BAT_SW_EN_PORT         PORT1
#define BAT_SW_EN_PIN          9U

#define BLUE_DRV_GPIO          GPIO1
#define BLUE_DRV_PORT          PORT1
#define BLUE_DRV_PIN           3U

#define GREEN_DRV_GPIO         GPIO1
#define GREEN_DRV_PORT         PORT1
#define GREEN_DRV_PIN          2U

#define RED_DRV_GPIO           GPIO1
#define RED_DRV_PORT           PORT1
#define RED_DRV_PIN            1U

#define CH_PWM_SYNC_GPIO       GPIO1
#define CH_PWM_SYNC_PORT       PORT1
#define CH_PWM_SYNC_PIN        0U

#define ON_OFF_SW_GPIO         GPIO1
#define ON_OFF_SW_PORT         PORT1
#define ON_OFF_SW_PIN          8U

#define MAX_CHARGE_CURRENT     950
#define ON_CHARGE_CURRENT      475
#define TRICKLE_CHARGE_CURRENT 75

#define MIN_BATTERY_SOC        20
#define MIN_BATTERY_VOLTAGE_V  13

#define EEPROM_SAVE_TIME_MS    60 * 1000 * 5
/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

#define V_AUX_EN_SET()         GPIO_PinWrite(V_AUX_EN_GPIO, V_AUX_EN_PIN, 1)
#define V_AUX_EN_CLR()         GPIO_PinWrite(V_AUX_EN_GPIO, V_AUX_EN_PIN, 0)
#define V_AUX_EN_TGL()         GPIO_PortToggle(V_AUX_EN_GPIO, 1U << V_AUX_EN_PIN)

#define BAT_SW_EN_SET()        GPIO_PinWrite(BAT_SW_EN_GPIO, BAT_SW_EN_PIN, 1)
#define BAT_SW_EN_CLR()        GPIO_PinWrite(BAT_SW_EN_GPIO, BAT_SW_EN_PIN, 0)
#define BAT_SW_EN_TGL()        GPIO_PortToggle(BAT_SW_EN_GPIO, 1U << BAT_SW_EN_PIN)

#define BLUE_DRV_SET()         GPIO_PinWrite(BLUE_DRV_GPIO, BLUE_DRV_PIN, 1)
#define BLUE_DRV_CLR()         GPIO_PinWrite(BLUE_DRV_GPIO, BLUE_DRV_PIN, 0)
#define BLUE_DRV_TGL()         GPIO_PortToggle(BLUE_DRV_GPIO, 1U << BLUE_DRV_PIN)

#define GREEN_DRV_SET()        GPIO_PinWrite(GREEN_DRV_GPIO, GREEN_DRV_PIN, 1)
#define GREEN_DRV_CLR()        GPIO_PinWrite(GREEN_DRV_GPIO, GREEN_DRV_PIN, 0)
#define GREEN_DRV_TGL()        GPIO_PortToggle(GREEN_DRV_GPIO, 1U << GREEN_DRV_PIN)

#define RED_DRV_SET()          GPIO_PinWrite(RED_DRV_GPIO, RED_DRV_PIN, 1)
#define RED_DRV_CLR()          GPIO_PinWrite(RED_DRV_GPIO, RED_DRV_PIN, 0)
#define RED_DRV_TGL()          GPIO_PortToggle(RED_DRV_GPIO, 1U << RED_DRV_PIN)

#define CH_PWM_SYNC_SET()      GPIO_PinWrite(CH_PWM_SYNC_GPIO, CH_PWM_SYNC_PIN, 1)
#define CH_PWM_SYNC_CLR()      GPIO_PinWrite(CH_PWM_SYNC_GPIO, CH_PWM_SYNC_PIN, 0)
#define CH_PWM_SYNC_TGL()      GPIO_PortToggle(CH_PWM_SYNC_GPIO, 1U << CH_PWM_SYNC_PIN)

#define ON_OFF_SW_READ()       GPIO_PinRead(ON_OFF_SW_GPIO, ON_OFF_SW_PIN)

  /*****************************************************************************
* Module Typedefs
******************************************************************************/

  typedef struct {
  uint16_t soc;
  uint16_t vbat;
  uint16_t ibat;
  uint16_t tbat;
} batteryInfo_t;

// Active Object
typedef struct {
  QActive super;
  QTimeEvt timerEvt;
  batteryInfo_t battInfo;
} BatteryManager_t;

/* Fasce di temperatura (°C) */
static const int16_t temp_ranges[5][2] = {
  {-10, 0}, // -10 ≤ T < 0
  {0, 10},  // 0 ≤ T < 10
  {10, 20}, // 10 ≤ T < 20
  {20, 30}, // 20 ≤ T < 30
  {30, 45}  // 30 ≤ T < 45
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
  {17520, 17760, 19080},

  /* 0 ≤ T < 10°C */
  {17280, 17400, 18240},

  /* 10 ≤ T < 20°C */
  {17160, 17280, 17760},

  /* 20 ≤ T < 30°C */
  {17040, 17160, 17520},

  /* 30 ≤ T < 45°C */
  {16920, 17040, 17400}};


  volatile uint16_t debug_current = 600U;
/*****************************************************************************
* Function Prototypes
******************************************************************************/

void battery_manager_init(void);

static QState battery_manager_initial_state(BatteryManager_t* const me, void const* const par);
static QState battery_manager_initialize_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_startup_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_active_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_on_charge_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_trickle_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_on_state(BatteryManager_t* const me, QEvt const* const e);
static QState battery_manager_alarm_state(BatteryManager_t* const me, QEvt const* const e);

static QState battery_manager_debug_state(BatteryManager_t* const me, QEvt const* const e);

static void battery_manager_pin_init(void);
static bool is_charge_finished(batteryInfo_t battInfo, uint32_t charge_curr_mA);
static uint32_t current_to_pwm(uint32_t curr_mA);
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

  //battery_manager_pin_init();
  //QActive_subscribe(&me->super, ADC_BATTERY_INFO_SAMPLE_SIG);
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
      static const QEvt evt = QEVT_INITIALIZER(INITIALIZE_SIG); // lo farà l'EEPROM quando è ready
      QACTIVE_POST(AO_BatteryManager, &evt, 0U);
      status = Q_HANDLED();
      break;
    }

    case INITIALIZE_SIG: {
      bsp_adc_init();
      bsp_pwm_init();
      battery_manager_pin_init();
      //bsp_i2c_init(&me->super);
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
      CH_PWM_SYNC_SET();
      //CH_PWM_SYNC_CLR();
      bsp_pwm_set_duty(current_to_pwm(debug_current));
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
      BtnEvt const* btn = (BtnEvt const*)e;
      if (btn->id == 0) { // id_wc_on
        status = Q_TRAN(&battery_manager_on_charge_state);
      } else {
        status = Q_TRAN(&battery_manager_on_state);
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

/**
 * @brief State of the battery manager module.
 * @details This state is responsible for managing the battery charging process.
 *        It will transition to the trickle state upon receiving the ALARM_SIG event.
 *        It will transition back to the active state upon receiving the Q_EXIT_SIG event.
 */
static QState
battery_manager_on_charge_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      V_AUX_EN_SET();
      CH_PWM_SYNC_SET();
      GREEN_DRV_SET();
      bsp_pwm_init();
      bsp_pwm_set_duty(current_to_pwm(ON_CHARGE_CURRENT));
      QTimeEvt_armX(&me->timerEvt, EEPROM_SAVE_TIME_MS, 0);
      status = Q_HANDLED();
      break;
    }

    case ADC_BATTERY_INFO_SAMPLE_SIG: {
      me->battInfo.soc = Q_EVT_CAST(AdcInfoEvt)->soc;
      me->battInfo.ibat = Q_EVT_CAST(AdcInfoEvt)->ibat;
      me->battInfo.vbat = Q_EVT_CAST(AdcInfoEvt)->vbat;
      me->battInfo.tbat = Q_EVT_CAST(AdcInfoEvt)->tbat;

      if (is_charge_finished(me->battInfo, ON_CHARGE_CURRENT)) {
        status = Q_TRAN(&battery_manager_trickle_state);
      } else {
        status = Q_HANDLED();
      }
      break;
    }

    case TIMEOUT_SIG: {
      EepromEvt* evt = Q_NEW(EepromEvt, EEPROM_WRITE_SIG);
      /*calculate the mAh per 5 minutes */
      evt->data = ON_CHARGE_CURRENT * (5 / 60);
      QF_PUBLISH(&evt->super, me);
      QTimeEvt_armX(&me->timerEvt, EEPROM_SAVE_TIME_MS, 0);
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
 * @details This state is responsible for managing the battery trickle charging process.
 *        It will transition to the active state upon receiving the Q_EXIT_SIG event.
 */
static QState
battery_manager_trickle_state(BatteryManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      BLUE_DRV_SET();
      bsp_pwm_set_duty(current_to_pwm(TRICKLE_CHARGE_CURRENT));
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
      bsp_pwm_deinit();
      BAT_SW_EN_SET();
      BLUE_DRV_SET();
      status = Q_HANDLED();
      break;
    }

    case ADC_BATTERY_INFO_SAMPLE_SIG: {
      me->battInfo.soc = Q_EVT_CAST(AdcInfoEvt)->soc;
      me->battInfo.ibat = Q_EVT_CAST(AdcInfoEvt)->ibat;
      me->battInfo.vbat = Q_EVT_CAST(AdcInfoEvt)->vbat;
      me->battInfo.tbat = Q_EVT_CAST(AdcInfoEvt)->tbat;

      if (me->battInfo.vbat < (MIN_BATTERY_VOLTAGE_V * 1000)) {
        status = Q_HANDLED();
      } else {
        static QEvt const evt = QEVT_INITIALIZER(OFF_SIG);
        QACTIVE_POST(AO_BatteryManager, &evt, 0U);
      }
      status = Q_HANDLED();
      break;
    }

    case BUTTON_PRESSED_SIG: {
      BtnEvt const* btn = (BtnEvt const*)e;
      if (btn->id == 0) {
        BAT_SW_EN_CLR();
        BLUE_DRV_CLR();
      } else {
        BAT_SW_EN_CLR();
      }
      status = Q_HANDLED();
      break;
    }

    case OFF_SIG: {
      BAT_SW_EN_CLR();
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
      BAT_SW_EN_CLR();
      CH_PWM_SYNC_CLR();
      //pwm a zero
      RED_DRV_SET();
      QTimeEvt_armX(&me->timerEvt, 5000, 0);
      status = Q_HANDLED();
      break;
    }

    case TIMEOUT_SIG: {
      BAT_SW_EN_CLR();
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
 * @brief Initialize the pins of the battery manager.
 *
 * @details This function configures the pins used by the battery manager.
 */
static void
battery_manager_pin_init() {
  /* Abilita clock delle porte GPIO coinvolte */
  CLOCK_EnableClock(kCLOCK_GatePORT1);
  CLOCK_EnableClock(kCLOCK_GateGPIO1);
  CLOCK_EnableClock(kCLOCK_GatePORT3);
  CLOCK_EnableClock(kCLOCK_GateGPIO3);

  /* Rilascia reset delle porte */
  RESET_ReleasePeripheralReset(kPORT1_RST_SHIFT_RSTn);
  RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);
  RESET_ReleasePeripheralReset(kGPIO1_RST_SHIFT_RSTn);
  RESET_ReleasePeripheralReset(kGPIO3_RST_SHIFT_RSTn);
  /************************************************************
     *  CONFIGURAZIONE PIN DI USCITA
     ************************************************************/
  const gpio_pin_config_t out_cfg = {
      .pinDirection = kGPIO_DigitalOutput,
      .outputLogic  = 0u
  };

  /* --------------------- V_AUX_EN (P3_0) --------------------- */
  const port_pin_config_t port3_0_cfg = {
    kPORT_PullDown,           kPORT_LowPullResistor,  kPORT_FastSlewRate,        kPORT_PassiveFilterDisable,
    kPORT_OpenDrainDisable,   kPORT_LowDriveStrength, kPORT_NormalDriveStrength, kPORT_MuxAsGpio,
    kPORT_InputBufferDisable, kPORT_InputNormal,      kPORT_UnlockRegister};
  PORT_SetPinConfig(PORT3, V_AUX_EN_PIN, &port3_0_cfg);
  GPIO_PinInit(GPIO3, V_AUX_EN_PIN, &out_cfg);

  /* --------------------- BAT_SW_EN (P1_9) --------------------- */
  const port_pin_config_t port1_9_cfg = {
    kPORT_PullDown,           kPORT_LowPullResistor,  kPORT_FastSlewRate,        kPORT_PassiveFilterDisable,
    kPORT_OpenDrainDisable,   kPORT_LowDriveStrength, kPORT_NormalDriveStrength, kPORT_MuxAsGpio,
    kPORT_InputBufferDisable, kPORT_InputNormal,      kPORT_UnlockRegister};
  PORT_SetPinConfig(PORT1, BAT_SW_EN_PIN, &port1_9_cfg);
  GPIO_PinInit(GPIO1, BAT_SW_EN_PIN, &out_cfg);

  /* --------------------- BLUE_DRV (P1_3) --------------------- */
  const port_pin_config_t port1_3_cfg = {
    kPORT_PullDown,           kPORT_LowPullResistor,  kPORT_FastSlewRate,        kPORT_PassiveFilterDisable,
    kPORT_OpenDrainDisable,   kPORT_LowDriveStrength, kPORT_NormalDriveStrength, kPORT_MuxAsGpio,
    kPORT_InputBufferDisable, kPORT_InputNormal,      kPORT_UnlockRegister};
  PORT_SetPinConfig(PORT1, BLUE_DRV_PIN, &port1_3_cfg);
  GPIO_PinInit(GPIO1, BLUE_DRV_PIN, &out_cfg);

  /* --------------------- GREEN_DRV (P1_2) --------------------- */
  const port_pin_config_t port1_2_cfg = {
    kPORT_PullDown,           kPORT_LowPullResistor,  kPORT_FastSlewRate,        kPORT_PassiveFilterDisable,
    kPORT_OpenDrainDisable,   kPORT_LowDriveStrength, kPORT_NormalDriveStrength, kPORT_MuxAsGpio,
    kPORT_InputBufferDisable, kPORT_InputNormal,      kPORT_UnlockRegister};
  PORT_SetPinConfig(PORT1, GREEN_DRV_PIN, &port1_2_cfg);
  GPIO_PinInit(GPIO1, GREEN_DRV_PIN, &out_cfg);

  /* --------------------- RED_DRV (P1_1) --------------------- */
  const port_pin_config_t port1_1_cfg = {
    kPORT_PullDown,           kPORT_LowPullResistor,  kPORT_FastSlewRate,        kPORT_PassiveFilterDisable,
    kPORT_OpenDrainDisable,   kPORT_LowDriveStrength, kPORT_NormalDriveStrength, kPORT_MuxAsGpio,
    kPORT_InputBufferDisable, kPORT_InputNormal,      kPORT_UnlockRegister};
  PORT_SetPinConfig(PORT1, RED_DRV_PIN, &port1_1_cfg);
  GPIO_PinInit(GPIO1, RED_DRV_PIN, &out_cfg);

  /* --------------------- CH_PWM_SYNC (P1_29) --------------------- */
  const port_pin_config_t port1_29_cfg = {
    kPORT_PullDown,           kPORT_LowPullResistor,  kPORT_FastSlewRate,        kPORT_PassiveFilterDisable,
    kPORT_OpenDrainDisable,   kPORT_LowDriveStrength, kPORT_NormalDriveStrength, kPORT_MuxAsGpio,
    kPORT_InputBufferDisable, kPORT_InputNormal,      kPORT_UnlockRegister};
  PORT_SetPinConfig(PORT1, CH_PWM_SYNC_PIN, &port1_29_cfg);
  GPIO_PinInit(GPIO1, CH_PWM_SYNC_PIN, &out_cfg);

  /************************************************************
     *  CONFIGURAZIONE PIN DI INGRESSO
     ************************************************************/

  /* --------------------- ON_OFF_SW (P1_8) --------------------- */
  const port_pin_config_t port1_8_cfg = {kPORT_PullUp,
                                         kPORT_LowPullResistor,
                                         kPORT_FastSlewRate,
                                         kPORT_PassiveFilterDisable,
                                         kPORT_OpenDrainDisable,
                                         kPORT_LowDriveStrength,
                                         kPORT_NormalDriveStrength,
                                         kPORT_MuxAsGpio,
                                         kPORT_InputBufferEnable,
                                         kPORT_InputNormal,
                                         kPORT_UnlockRegister};
  PORT_SetPinConfig(PORT1, ON_OFF_SW_PIN, &port1_8_cfg);
  GPIO_PinInit(GPIO1, ON_OFF_SW_PIN, &(gpio_pin_config_t){kGPIO_DigitalInput, 0});
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
static bool
is_charge_finished(batteryInfo_t battInfo, uint32_t charge_curr_mA) {
  int i, j;

  /* Trova indice fascia temperatura */
  for (i = 0; i < 5; i++) {
    if (battInfo.tbat >= temp_ranges[i][0] && battInfo.tbat < temp_ranges[i][1]) {
      break;
    }
  }
  if (i >= 5) {
    return false;
  }

  /* Trova indice fascia corrente (It rate) */
  for (j = 0; j < 3; j++) {
    if (charge_curr_mA >= current_ranges_mA[j][0] && charge_curr_mA <= current_ranges_mA[j][1]) {
      break;
    }
  }
  if (j >= 3) {
    return false;
  }

  uint16_t termination_mV = termination_voltage_charge_mV[i][j];

  /* Controllo terminazione */
  if (battInfo.vbat >= termination_mV) {
    return true;
  }

  return false;
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
