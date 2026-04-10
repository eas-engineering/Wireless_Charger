/******************************************************************************
* Filename              :   bsp_keypad.c
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   19 September 2024
*
* Copyright (c) 2024 EAS SPA. All rights reserved.  
*
******************************************************************************/

/** @file bsp_keypad.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include "bsp_keypad.h"
#include "bsp_input_dc.h"
#include "fsl_common.h"
#include "fsl_ctimer.h"
#include "fsl_gpio.h"
#include "fsl_inputmux.h"
#include "fsl_lpadc.h"
#include "fsl_port.h"
#include "global_signals.h"
#include "project_settings.h"

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/

#define ON_OFF_SW_GPIO  GPIO1
#define ON_OFF_SW_PORT  PORT1
#define ON_OFF_SW_PIN   8U

#define CHARGE_BTN_GPIO GPIO3
#define CHARGE_BTN_PORT PORT3
#define CHARGE_BTN_PIN  3U

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

/*****************************************************************************
* Module Typedefs
******************************************************************************/

/*****************************************************************************
* Function Prototypes
******************************************************************************/

static void initialize_keypad_gpio(bsp_input_dc_cfg_t const* config, int32_t num);

static void button_callback(struct bsp_button* const _this, bsp_button_event_t event);

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*
   * Ingressi digitali collegati ai pulsanti
   */
bsp_input_dc_t UserButtonInput[BSP_MAX_KEYPAD_BTN];

/*
   * Istanza dei pulsanti
   */
bsp_button_t UserButton[BSP_MAX_KEYPAD_BTN];

#ifdef Q_SPY
static QSpyId const l_button_callback = {0U};
#endif

/*****************************************************************************
* Function Definitions
******************************************************************************/

/**
 * @brief This function initialize the keypad. Create the input line object and
 *        buttons objects in order to manage the keypad events.
 * 
 * @param  
 */
void
keypad_init(void) {

  bsp_button_cfg_t bsp_button_cfg;
  bsp_input_dc_cfg_t input_config[BSP_MAX_KEYPAD_BTN] = {
    [BSP_KEYPAD_ON_OFF_BTN] = {.port = ON_OFF_SW_PORT,
                               .gpio = ON_OFF_SW_GPIO,
                               .pin = ON_OFF_SW_PIN,
                               .active_state = BSP_INPUT_DC_ACTIVE_LOW_STATE,
                               .ui32_debounce_time_ms = 30},
    [BSP_KEYPAD_CHARGE_BTN] = {.port = CHARGE_BTN_PORT,
                               .gpio = CHARGE_BTN_GPIO,
                               .pin = CHARGE_BTN_PIN,
                               .active_state = BSP_INPUT_DC_ACTIVE_LOW_STATE,
                               .ui32_debounce_time_ms = 30},
  };

  initialize_keypad_gpio(input_config, BSP_MAX_KEYPAD_BTN);

  /* Inizializzo le linee digitali collegate ai pulsanti */
  for (uint32_t idxLine = 0U; idxLine < BSP_MAX_KEYPAD_BTN; ++idxLine) {
    bsp_input_dc_add(&(UserButtonInput[idxLine]), &(input_config[idxLine]));
  }

  /* Inzializzo i pulsanti */
  bsp_button_cfg.ui32_normal_press_time = 30;
  bsp_button_cfg.ui32_long_press_time = 200;
  for (int32_t idxBtn = 0; idxBtn < BSP_MAX_KEYPAD_BTN; ++idxBtn) {
    bsp_button_cfg.p_input = (bsp_input_t*)&(UserButtonInput[idxBtn]);
    bsp_button_cfg.id = idxBtn;
    bsp_button_add(&(UserButton[idxBtn]), &bsp_button_cfg);
    bsp_button_set_callback(&(UserButton[idxBtn]), button_callback);
  }

  QS_OBJ_DICTIONARY(&l_button_callback);
}

/**
 * @brief Get the state of the button
 * 
 * This function returns the state of the button
 * 
 * @param btn Button to check
 * @return State of the button
 */
bsp_button_state_t
keypad_get_button_state(bsp_keypad_btn_t btn) {
  bsp_button_state_t state = BSP_BUTTON_RELEASED_STATE;
  bsp_button_get_state(&(UserButton[btn]), &state);
  return state;
}

/**
 * @brief Set the long press time for the button
 * 
 * This function sets the time for a long press event
 * 
 * @param btn Button to set
 * @param time Time in ms for long press
 */
void
bsp_keypad_set_long_press_time(bsp_keypad_btn_t btn, uint32_t time) {
  QF_CRIT_ENTRY();
  bsp_button_set_long_press_time(&(UserButton[btn]), time);
  QF_CRIT_EXIT();
}

/**
 * @brief This function initialize the hardware GPIO line used by
 *        the keypad.
 * 
 * @param config: input configuration
 * @param num: number of input
 */
static void
initialize_keypad_gpio(bsp_input_dc_cfg_t const* config_ptr, int32_t num) {

  port_pin_config_t port1_8_cfg = {kPORT_PullUp,
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

  /* Abilita clock delle porte GPIO coinvolte */
  CLOCK_EnableClock(kCLOCK_GatePORT1);
  CLOCK_EnableClock(kCLOCK_GateGPIO1);
  CLOCK_EnableClock(kCLOCK_GatePORT3);
  CLOCK_EnableClock(kCLOCK_GateGPIO3);

  /* Rilascia reset delle porte */
  RESET_ReleasePeripheralReset(kPORT1_RST_SHIFT_RSTn);
  RESET_ReleasePeripheralReset(kGPIO1_RST_SHIFT_RSTn);
  RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);
  RESET_ReleasePeripheralReset(kGPIO3_RST_SHIFT_RSTn);

  for (int32_t idxLine = 0U; idxLine < num; ++idxLine) {
    PORT_SetPinConfig(config_ptr->port, config_ptr->pin, &port1_8_cfg);
    GPIO_PinInit(config_ptr->gpio, config_ptr->pin, &(gpio_pin_config_t){kGPIO_DigitalInput, 0});
    ++config_ptr;
  }
}

/**
 * @brief This function is call from the button class when a new
 *        event is generated.
 * 
 * @param _me: pointer to the button
 * @param event: button event
 */
static void
button_callback(struct bsp_button* const _me, bsp_button_event_t event) {

  QK_ISR_ENTRY(); // inform QK about entering an ISR

  keypad_event_t* evt = Q_NEW(keypad_event_t, BUTTON_PRESSED_SIG);
  evt->btn = _me->id;
  evt->event = event;
  QF_PUBLISH((QEvt*)evt, &l_button_callback);

  QK_ISR_EXIT(); // inform QK about exiting an ISR
}