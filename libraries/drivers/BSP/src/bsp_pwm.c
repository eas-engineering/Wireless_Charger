/******************************************************************************
 * Filename              : bsp_pwm.c
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

/** @file bsp_pwm.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/

#include "bsp_pwm.h"
#include "fsl_common.h"
#include "fsl_ctimer.h"
#include "fsl_gpio.h"
#include "fsl_inputmux.h"
#include "fsl_lpadc.h"
#include "fsl_port.h"
#include "project_settings.h"

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/

#define CTIMER                        CTIMER1         /* Timer 1 */
#define CTIMER_MAT_OUT                kCTIMER_Match_0 /* Match output 2 */
#define CTIMER_CLK_FREQ               CLOCK_GetCTimerClkFreq(1U)
#define CTIMER_MAT_PWM_PERIOD_CHANNEL kCTIMER_Match_1

#define CH_CURR_SET_GPIO              GPIO3
#define CH_CURR_SET_PORT              PORT3
#define CH_CURR_SET_PIN               10U

//#define CH_PWM_SYNC_GPIO       GPIO1
//#define CH_PWM_SYNC_PORT       PORT1
//#define CH_PWM_SYNC_PIN        0U

#define PWM_20_KHZ             32000U
#define PWM_700_KHZ            600000U

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

static void bsp_pwm_pin_init(void);
static void bsp_pwm_timer_init(void);
static void bsp_pwm_sync_timer_init(void);
/*****************************************************************************
* Function Definitions
******************************************************************************/

/**
 * @brief This function is used to initialize the PWM module.
 *
 * It initializes the PWM pin and starts the PWM timer.
 */
void
bsp_pwm_init(void) {
  bsp_pwm_pin_init();
  bsp_pwm_timer_init();
  //bsp_pwm_sync_timer_init();
}

/**
 * @brief This function is used to set the duty cycle of the PWM signal.
 *
 * @param duty_percent The duty cycle of the PWM signal in percentage, valid range is 0 to 100.
 */
void
bsp_pwm_set_duty(uint32_t duty_percent) {

  if (duty_percent > 100U) {
    duty_percent = 100U;
  }

  CTIMER_UpdatePwmDutycycle(CTIMER, CTIMER_MAT_PWM_PERIOD_CHANNEL, CTIMER_MAT_OUT, duty_percent);
}

/**
 * @brief This function is used to initialize the board PWM pin settings.
 *
 * The function is used to configure the PWM0_A0 pin on PORT3.
 *
 * The pin is configured as an output with low drive strength and without internal pull-up/down resistor.
 * The pin is also configured with fast slew rate and with passive input filter disabled.
 * Open drain output is disabled and the pin is configured with normal drive strength.
 * The pin is configured as PWM0_A0 with digital input enabled and not inverted.
 */
static void
bsp_pwm_pin_init(void) {

  /* Write to PORT3: Peripheral clock is enabled */
  CLOCK_EnableClock(kCLOCK_GatePORT3);
  //CLOCK_EnableClock(kCLOCK_GatePORT1);

  /* PORT3 peripheral is released from reset */
  RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);
  //RESET_ReleasePeripheralReset(kPORT1_RST_SHIFT_RSTn);
  const port_pin_config_t port_config = {/* Internal pull-up/down resistor is disabled */
                                         kPORT_PullDisable,
                                         /* Low internal pull resistor value is selected. */
                                         kPORT_LowPullResistor,
                                         /* Fast slew rate is configured */
                                         kPORT_FastSlewRate,
                                         /* Passive input filter is disabled */
                                         kPORT_PassiveFilterDisable,
                                         /* Open drain output is disabled */
                                         kPORT_OpenDrainDisable,
                                         /* Low drive strength is configured */
                                         kPORT_LowDriveStrength,
                                         /* Normal drive strength is configured */
                                         kPORT_NormalDriveStrength,
                                         /* Pin is configured as CT1_MAT0 */
                                         kPORT_MuxAlt4,
                                         /* Digital input enabled */
                                         kPORT_InputBufferEnable,
                                         /* Digital input is not inverted */
                                         kPORT_InputNormal,
                                         /* Pin Control Register fields [15:0] are not locked */
                                         kPORT_UnlockRegister};

// const port_pin_config_t port_config_pwm_sync = {/* Internal pull-up/down resistor is disabled */
//                                          kPORT_PullDisable,
//                                          /* Low internal pull resistor value is selected. */
//                                          kPORT_LowPullResistor,
//                                          /* Fast slew rate is configured */
//                                          kPORT_FastSlewRate,
//                                          /* Passive input filter is disabled */
//                                          kPORT_PassiveFilterDisable,
//                                          /* Open drain output is disabled */
//                                          kPORT_OpenDrainDisable,
//                                          /* Low drive strength is configured */
//                                          kPORT_LowDriveStrength,
//                                          /* Normal drive strength is configured */
//                                          kPORT_NormalDriveStrength,
//                                          /* Pin is configured as CT2_MAT0 */
//                                          kPORT_MuxAlt5,
//                                          /* Digital input enabled */
//                                          kPORT_InputBufferEnable,
//                                          /* Digital input is not inverted */
//                                          kPORT_InputNormal,
//                                          /* Pin Control Register fields [15:0] are not locked */
//                                          kPORT_UnlockRegister};

  /* PORT3_10 (pin 19) is configured as PWM0_A0 */
  PORT_SetPinConfig(CH_CURR_SET_PORT, CH_CURR_SET_PIN, &port_config);
  /* CH_PWM_SYNC_PIN (pin 0) is configured as GPIO */
  //PORT_SetPinConfig(CH_PWM_SYNC_PORT, CH_PWM_SYNC_PIN, &port_config_pwm_sync);
}

/**
 * @brief Initializes the CTIMER for PWM generation.
 *
 * This function initializes the CTIMER peripheral, sets up the PWM signal
 * generation, and starts the CTIMER timer.
 */
static void
bsp_pwm_timer_init(void) {

  ctimer_config_t config;
  uint32_t timerClock;

  CLOCK_SetClockDiv(kCLOCK_DivCTIMER1, 1u);
  CLOCK_AttachClk(kFRO_HF_to_CTIMER1);

  /* CTIMER1 peripheral is released from reset */
  RESET_ReleasePeripheralReset(kCTIMER1_RST_SHIFT_RSTn);

  CTIMER_GetDefaultConfig(&config);
  CTIMER_Init(CTIMER, &config);

  timerClock = CLOCK_GetCTimerClkFreq(1U) / (config.prescale + 1);
  /*20[kHz]*/
  CTIMER_SetupPwm(CTIMER, CTIMER_MAT_PWM_PERIOD_CHANNEL, CTIMER_MAT_OUT, 0U, PWM_20_KHZ*2, timerClock, false);
  CTIMER_StartTimer(CTIMER);
}

/**
 * @brief Initializes the CTIMER for PWM generation on sync pin.
 *
 * This function initializes the CTIMER2 peripheral, sets up the PWM signal
 * generation at 700 kHz, and starts the CTIMER timer.
 */
static void
bsp_pwm_sync_timer_init() {

  ctimer_config_t config;
  uint32_t timerClock;

  CLOCK_SetClockDiv(kCLOCK_DivCTIMER0, 1u);
  CLOCK_AttachClk(kFRO_HF_to_CTIMER0);

  /* CTIMER2 peripheral is released from reset */
  RESET_ReleasePeripheralReset(kCTIMER0_RST_SHIFT_RSTn);

  CTIMER_GetDefaultConfig(&config);
  CTIMER_Init(CTIMER0, &config);

  timerClock = CLOCK_GetCTimerClkFreq(1U) / (config.prescale + 1);
  /*700[kHz]*/
  CTIMER_SetupPwm(CTIMER0, kCTIMER_Match_0, kCTIMER_Match_2, 0U, PWM_700_KHZ, timerClock, false);
  CTIMER_StartTimer(CTIMER0);
  CTIMER_UpdatePwmDutycycle(CTIMER0, kCTIMER_Match_0, kCTIMER_Match_2, 50U);
}

void
bsp_pwm_deinit(void) {
  // Ferma il timer PWM
  CTIMER_StopTimer(CTIMER);
  //CTIMER_StopTimer(CTIMER0);
  // Rimappa il pin come GPIO normale
  const port_pin_config_t gpio_cfg = {
    kPORT_PullDisable,        kPORT_LowPullResistor,  kPORT_FastSlewRate,        kPORT_PassiveFilterDisable,
    kPORT_OpenDrainDisable,   kPORT_LowDriveStrength, kPORT_NormalDriveStrength,
    kPORT_MuxAsGpio, // <-- GPIO
    kPORT_InputBufferDisable, kPORT_InputNormal,      kPORT_UnlockRegister};
  PORT_SetPinConfig(CH_CURR_SET_PORT, CH_CURR_SET_PIN, &gpio_cfg);
  //PORT_SetPinConfig(CH_PWM_SYNC_PORT, CH_PWM_SYNC_PIN, &gpio_cfg);
  // Imposta stato del pin quando PWM è OFF (qui LOW)
  gpio_pin_config_t out_cfg = {kGPIO_DigitalOutput, 0};
  GPIO_PinInit(CH_CURR_SET_GPIO, CH_CURR_SET_PIN, &out_cfg);
  GPIO_PinWrite(CH_CURR_SET_GPIO, CH_CURR_SET_PIN, 0); // forzato low
  //GPIO_PinInit(CH_PWM_SYNC_GPIO, CH_PWM_SYNC_PIN, &out_cfg);
  //GPIO_PinWrite(CH_PWM_SYNC_GPIO, CH_PWM_SYNC_PIN, 0); // forzato low
}