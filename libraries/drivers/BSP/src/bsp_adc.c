/******************************************************************************
 * Filename              : bsp_adc.c
 * Author                : Giulio Dalla Vecchia
 * Origin Date           : 30 December 2025
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

/** @file bsp_adc.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include "bsp_adc.h"
#include "bsp_led.h"
#include "bsp_ntc_battery.h"
#include "fsl_common.h"
#include "fsl_ctimer.h"
#include "fsl_inputmux.h"
#include "fsl_lpadc.h"
#include "fsl_port.h"
#include "project_settings.h"
#include "qpc.h"
#include "soc_NiMh.h"

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/

#define LPADC_BASE                    ADC0
#define LPADC_VOLTAGE_BATTERY_CHANNEL 4U
#define LPADC_VOLTAGE_BATTERY_CMDID   1U /* CMD1 */

#define LPADC_TEMPERATURE_CHANNEL     2U
#define LPADC_TEMPERATURE_CMDID       2U /* CMD2 */

#define LPADC_CURRENT_CHANNEL         7U
#define LPADC_CURRENT_CMDID           3U /* CMD3 */

#define LPADC_IRQn                    ADC0_IRQn

#define CTIMER                        CTIMER2         /* Timer 1 */
#define CTIMER_MAT_OUT                kCTIMER_Match_0 /* Match output 2 */
#define CTIMER_CLK_FREQ               CLOCK_GetCTimerClkFreq(1U)
#define CTIMER_MAT_PWM_PERIOD_CHANNEL kCTIMER_Match_1

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

/*****************************************************************************
* Module Typedefs
******************************************************************************/

/*****************************************************************************
* Function Prototypes
******************************************************************************/

static void bsp_adc_pin_init(void);
static void bsp_adc_peripheral_init(void);
static void bsp_adc_timer_trigger_init(void);
static void bsp_adc_input_mux_init(void);

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*****************************************************************************
* Function Definitions
******************************************************************************/

/**
 * @brief Initializes the ADC peripheral, input mux and timer trigger.
 *
 * This function initializes the ADC peripheral, configures the input mux
 * and enables the timer trigger.
 */
void
bsp_adc_init(void) {
  bsp_adc_pin_init();
  bsp_adc_peripheral_init();
  bsp_adc_timer_trigger_init();
  bsp_adc_input_mux_init();
}

void
bsp_adc_isr_handler(void) {

  lpadc_conv_result_t adc_result;
  uint32_t voltage_mV;
  uint32_t current_mA;
  int16_t temp;

  if (LPADC_GetConvResult(LPADC_BASE, &adc_result)) {}

  // 16-bit conversion result
  voltage_mV = (adc_result.convValue * 3307U) / 65535U;

  if (LPADC_GetConvResult(LPADC_BASE, &adc_result)) {}

  // 16-bit conversion result
  temp = (int16_t)bsp_ntc_battery_get_temperature(adc_result.convValue, 65535) * 100U;

  if (LPADC_GetConvResult(LPADC_BASE, &adc_result)) {}

  // 16-bit conversion result
  current_mA = (adc_result.convValue * 3307U) / 65535U;

  QK_ISR_ENTRY();
  AdcInfoEvt* evt = Q_NEW(AdcInfoEvt, ADC_BATTERY_INFO_SAMPLE_SIG);
  evt->vbat = voltage_mV;
  evt->ibat = current_mA;
  evt->tbat = temp;
  QF_PUBLISH(&evt->super, 0);
  QK_ISR_EXIT();

  SDK_ISR_EXIT_BARRIER;
}

/**
 * @brief Configures the ADC pins for the MCU.
 *
 * This function configures the ADC0_A0 and ADC0_A4 pins for the MCU.
 *
 * @note This function should be called before the ADC is initialized.
 */
static void
bsp_adc_pin_init(void) {

  /* Write to PORT3: Peripheral clock is enabled */
  CLOCK_EnableClock(kCLOCK_GatePORT2);

  /* PORT3 peripheral is released from reset */
  RESET_ReleasePeripheralReset(kPORT2_RST_SHIFT_RSTn);

  const port_pin_config_t pin_config = {/* Internal pull-up/down resistor is disabled */
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
                                        /* Pin is configured as ADC0_A0 */
                                        kPORT_MuxAlt0,
                                        /* Digital input disabled; it is required for analog functions */
                                        kPORT_InputBufferDisable,
                                        /* Digital input is not inverted */
                                        kPORT_InputNormal,
                                        /* Pin Control Register fields [15:0] are not locked */
                                        kPORT_UnlockRegister};

  /* PORT2_2 (pin 8) is configured as ADC0_A4 */
  PORT_SetPinConfig(PORT2, 2U, &pin_config);

  /* PORT2_3 (pin 9) is configured as ADC0_A2 */
  PORT_SetPinConfig(PORT2, 3U, &pin_config);

  /* PORT2_7 (pin 11) is configured as ADC0_A7 */
  PORT_SetPinConfig(PORT2, 7U, &pin_config);
}

/**
 * @brief Initializes the ADC peripheral with the default configuration.
 *
 * This function initializes the ADC peripheral with the default configuration
 * and sets the conversion commands for the voltage battery and temperature
 * channels.
 *
 * @note This function should be called before the ADC is used.
 */
static void
bsp_adc_peripheral_init(void) {
  lpadc_config_t adc_config;
  lpadc_conv_command_config_t command_config;
  lpadc_conv_trigger_config_t adc_trigger_config;

  /* Attach peripheral clock */
  CLOCK_SetClockDiv(kCLOCK_DivADC0, 1u);
  CLOCK_AttachClk(kFRO12M_to_ADC0);

  /* ADC0 peripheral is released from reset */
  RESET_ReleasePeripheralReset(kADC0_RST_SHIFT_RSTn);

  LPADC_GetDefaultConfig(&adc_config);
  adc_config.enableAnalogPreliminary = true;
  adc_config.referenceVoltageSource = kLPADC_ReferenceVoltageAlt3; // VDD_ANA supply pin
  adc_config.conversionAverageMode = kLPADC_ConversionAverage1024;
  adc_config.FIFOWatermark = 2U; // Interrupt is generated when 3 conversion results are in the FIFO
  LPADC_Init(LPADC_BASE, &adc_config);

  LPADC_DoOffsetCalibration(LPADC_BASE);
  LPADC_DoAutoCalibration(LPADC_BASE);

  /* Set conversion CMD configuration. */
  LPADC_GetDefaultConvCommandConfig(&command_config);
  command_config.channelNumber = LPADC_VOLTAGE_BATTERY_CHANNEL;
  command_config.conversionResolutionMode = kLPADC_ConversionResolutionHigh;
  command_config.sampleTimeMode = kLPADC_SampleTimeADCK67;
  command_config.chainedNextCommandNumber = LPADC_TEMPERATURE_CMDID;
  LPADC_SetConvCommandConfig(LPADC_BASE, LPADC_VOLTAGE_BATTERY_CMDID, &command_config);

  command_config.channelNumber = LPADC_TEMPERATURE_CHANNEL;
  command_config.chainedNextCommandNumber = LPADC_CURRENT_CMDID;
  LPADC_SetConvCommandConfig(LPADC_BASE, LPADC_TEMPERATURE_CMDID, &command_config);

  command_config.channelNumber = LPADC_CURRENT_CHANNEL;
  command_config.chainedNextCommandNumber = 0; /* No next command defined. */
  LPADC_SetConvCommandConfig(LPADC_BASE, LPADC_CURRENT_CMDID, &command_config);

  /* Set trigger configuration. */
  LPADC_GetDefaultConvTriggerConfig(&adc_trigger_config);
  adc_trigger_config.targetCommandId = LPADC_VOLTAGE_BATTERY_CMDID; /* LPADC_VOLTAGE_BATTERY_CMDID is executed. */
  adc_trigger_config.enableHardwareTrigger = true;
  LPADC_SetConvTriggerConfig(LPADC_BASE, 0U, &adc_trigger_config); /* Configurate the trigger0. */

  LPADC_EnableInterrupts(LPADC_BASE, kLPADC_FIFOWatermarkInterruptEnable);
  EnableIRQ(LPADC_IRQn);
}

/**
 * @brief Initializes the CTIMER for generating the ADC trigger.
 *
 * This function initializes the CTIMER peripheral, sets up the PWM signal
 * generation, and starts the CTIMER timer. The generated PWM signal is used
 * as the hardware trigger for starting ADC conversions.
 *
 * @note This function should be called before the ADC is used.
 */
static void
bsp_adc_timer_trigger_init(void) {

  ctimer_config_t config;

  CLOCK_SetClockDiv(kCLOCK_DivCTIMER2, 1u);
  CLOCK_AttachClk(kFRO_HF_to_CTIMER2);

  CTIMER_GetDefaultConfig(&config);

  /* Imposto prescaler per avere 1000 Hz */
  config.prescale = 47999; // (48MHz / 48000 = 1000 Hz)

  CTIMER_Init(CTIMER, &config);

  /* Vogliamo un trigger ogni 0.5 secondo → period = 249 */
  uint32_t period = 249U;

  //CTIMER_SetupPwm(CTIMER, CTIMER_MAT_PWM_PERIOD_CHANNEL, CTIMER_MAT_OUT, 50U, period, timerClock, false);

  //Vogliamo 1 Hz = un MATCH ogni 1000 tick del timer (perché il timer ora è 1 kHz)
  CTIMER_SetupMatch(CTIMER, CTIMER_MAT_OUT,
                    &(ctimer_match_config_t){
                      .matchValue = period,
                      .enableCounterReset = true,          /* Reset counter when match occurs */
                      .enableInterrupt = false,            /* No interrupt needed for the match */
                      .outControl = kCTIMER_Output_Toggle, /* Toggle output on match */
                    });

  CTIMER_StartTimer(CTIMER);
}

/**
 * @brief Initializes the input multiplexer for routing the CTIMER2_MAT0 signal to the ADC0 trigger.
 *
 * This function initializes the input multiplexer peripheral, connects the CTIMER2_MAT0 signal to the ADC0 trigger,
 * and then deinitializes the input multiplexer peripheral.
 *
 * @note This function should be called before the ADC is used.
 */
static void
bsp_adc_input_mux_init(void) {

  INPUTMUX_Init(INPUTMUX0);

  /* Connect CTIMER2_MAT0 to ADC0_TRG0 */
  INPUTMUX_AttachSignal(INPUTMUX0, 0, kINPUTMUX_Ctimer2M0ToAdc0Trigger);

  INPUTMUX_Deinit(INPUTMUX0);
}