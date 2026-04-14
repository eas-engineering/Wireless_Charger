/******************************************************************************
 * Filename              : bsp_digital_output.c
 * Author                : Giulio Dalla Vecchia
 * Origin Date           : 10 April 2026
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

/** @file bsp_digital_output.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include "bsp_digital_output.h"
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

#define V_AUX_EN_GPIO     GPIO3
#define V_AUX_EN_PORT     PORT3
#define V_AUX_EN_PIN      0U

#define BAT_SW_EN_GPIO    GPIO1
#define BAT_SW_EN_PORT    PORT1
#define BAT_SW_EN_PIN     9U

#define CH_PWM_SYNC_GPIO  GPIO1
#define CH_PWM_SYNC_PORT  PORT1
#define CH_PWM_SYNC_PIN   0U

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

#define V_AUX_EN_SET()    GPIO_PinWrite(V_AUX_EN_GPIO, V_AUX_EN_PIN, 1)
#define V_AUX_EN_CLR()    GPIO_PinWrite(V_AUX_EN_GPIO, V_AUX_EN_PIN, 0)
#define V_AUX_EN_TGL()    GPIO_PortToggle(V_AUX_EN_GPIO, 1U << V_AUX_EN_PIN)

#define BAT_SW_EN_SET()   GPIO_PinWrite(BAT_SW_EN_GPIO, BAT_SW_EN_PIN, 1)
#define BAT_SW_EN_CLR()   GPIO_PinWrite(BAT_SW_EN_GPIO, BAT_SW_EN_PIN, 0)
#define BAT_SW_EN_TGL()   GPIO_PortToggle(BAT_SW_EN_GPIO, 1U << BAT_SW_EN_PIN)

#define CH_PWM_SYNC_SET() GPIO_PinWrite(CH_PWM_SYNC_GPIO, CH_PWM_SYNC_PIN, 1)
#define CH_PWM_SYNC_CLR() GPIO_PinWrite(CH_PWM_SYNC_GPIO, CH_PWM_SYNC_PIN, 0)
#define CH_PWM_SYNC_TGL() GPIO_PortToggle(CH_PWM_SYNC_GPIO, 1U << CH_PWM_SYNC_PIN)

/*****************************************************************************
* Module Typedefs
******************************************************************************/

/*****************************************************************************
* Function Prototypes
******************************************************************************/

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*****************************************************************************
* Function Definitions
******************************************************************************/

/**
 * @brief Inizializza i pin di uscita digitale.
 *
 * Abilita clock delle porte GPIO coinvolte, rilascia reset delle porte e
 * configura i pin di uscita digitale.
 */
void
bsp_digital_output_init(void) {

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
  const gpio_pin_config_t out_cfg = {.pinDirection = kGPIO_DigitalOutput, .outputLogic = 0u};

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

  /* --------------------- CH_PWM_SYNC (P1_29) --------------------- */
  const port_pin_config_t port1_0_cfg = {
    kPORT_PullDown,           kPORT_LowPullResistor,  kPORT_FastSlewRate,        kPORT_PassiveFilterDisable,
    kPORT_OpenDrainDisable,   kPORT_LowDriveStrength, kPORT_NormalDriveStrength, kPORT_MuxAsGpio,
    kPORT_InputBufferDisable, kPORT_InputNormal,      kPORT_UnlockRegister};
  PORT_SetPinConfig(PORT1, CH_PWM_SYNC_PIN, &port1_0_cfg);
  GPIO_PinInit(GPIO1, CH_PWM_SYNC_PIN, &out_cfg);
}

/**
 * @brief Set the state of a digital output.
 *
 * This function sets the state of a digital output based on the given ID and state.
 *
 * @param id The ID of the digital output to set.
 * @param state The state to set the digital output to.
 */
void
bsp_digital_output_set(io_id_t id, io_state_t state) {
  switch (id) {
    case IO_V_AUX_EN:
      if (state == IO_ON) {
        V_AUX_EN_SET();
      } else {
        V_AUX_EN_CLR();
      }
      break;
    case IO_BAT_SW_EN:
      if (state == IO_ON) {
        BAT_SW_EN_SET();
      } else {
        BAT_SW_EN_CLR();
      }
      break;
    case IO_CH_PWM_SYNC:
      if (state == IO_ON) {
        CH_PWM_SYNC_SET();
      } else {
        CH_PWM_SYNC_CLR();
      }
      break;
    default:
      /* Invalid ID, handle error if necessary */
      break;
  }
}