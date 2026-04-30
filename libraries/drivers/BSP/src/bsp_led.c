/******************************************************************************
 * Filename              : bsp_led.c
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

/** @file bsp_led.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include "bsp_led.h"
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

#define BLUE_DRV_GPIO   GPIO1
#define BLUE_DRV_PORT   PORT1
#define BLUE_DRV_PIN    3U

#define GREEN_DRV_GPIO  GPIO1
#define GREEN_DRV_PORT  PORT1
#define GREEN_DRV_PIN   2U

#define RED_DRV_GPIO    GPIO1
#define RED_DRV_PORT    PORT1
#define RED_DRV_PIN     1U

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

#define BLUE_DRV_SET()  GPIO_PinWrite(BLUE_DRV_GPIO, BLUE_DRV_PIN, 1)
#define BLUE_DRV_CLR()  GPIO_PinWrite(BLUE_DRV_GPIO, BLUE_DRV_PIN, 0)
#define BLUE_DRV_TGL()  GPIO_PortToggle(BLUE_DRV_GPIO, 1U << BLUE_DRV_PIN)

#define GREEN_DRV_SET() GPIO_PinWrite(GREEN_DRV_GPIO, GREEN_DRV_PIN, 1)
#define GREEN_DRV_CLR() GPIO_PinWrite(GREEN_DRV_GPIO, GREEN_DRV_PIN, 0)
#define GREEN_DRV_TGL() GPIO_PortToggle(GREEN_DRV_GPIO, 1U << GREEN_DRV_PIN)

#define RED_DRV_SET()   GPIO_PinWrite(RED_DRV_GPIO, RED_DRV_PIN, 1)
#define RED_DRV_CLR()   GPIO_PinWrite(RED_DRV_GPIO, RED_DRV_PIN, 0)
#define RED_DRV_TGL()   GPIO_PortToggle(RED_DRV_GPIO, 1U << RED_DRV_PIN)

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

void
bsp_led_init(void) {

  /* Abilita clock delle porte GPIO coinvolte */
  CLOCK_EnableClock(kCLOCK_GatePORT1);
  CLOCK_EnableClock(kCLOCK_GateGPIO1);

  /* Rilascia reset delle porte */
  RESET_ReleasePeripheralReset(kPORT1_RST_SHIFT_RSTn);
  RESET_ReleasePeripheralReset(kGPIO1_RST_SHIFT_RSTn);

  gpio_pin_config_t out_cfg = {.pinDirection = kGPIO_DigitalOutput, .outputLogic = 0u};

  /* --------------------- BLUE_DRV (P1_3) --------------------- */
  port_pin_config_t port1_3_cfg = {
    kPORT_PullDown,           kPORT_LowPullResistor,  kPORT_FastSlewRate,        kPORT_PassiveFilterDisable,
    kPORT_OpenDrainDisable,   kPORT_LowDriveStrength, kPORT_NormalDriveStrength, kPORT_MuxAsGpio,
    kPORT_InputBufferDisable, kPORT_InputNormal,      kPORT_UnlockRegister};
  PORT_SetPinConfig(PORT1, BLUE_DRV_PIN, &port1_3_cfg);
  GPIO_PinInit(GPIO1, BLUE_DRV_PIN, &out_cfg);

  /* --------------------- GREEN_DRV (P1_2) --------------------- */
  port_pin_config_t port1_2_cfg = {
    kPORT_PullDown,           kPORT_LowPullResistor,  kPORT_FastSlewRate,        kPORT_PassiveFilterDisable,
    kPORT_OpenDrainDisable,   kPORT_LowDriveStrength, kPORT_NormalDriveStrength, kPORT_MuxAsGpio,
    kPORT_InputBufferDisable, kPORT_InputNormal,      kPORT_UnlockRegister};
  PORT_SetPinConfig(PORT1, GREEN_DRV_PIN, &port1_2_cfg);
  GPIO_PinInit(GPIO1, GREEN_DRV_PIN, &out_cfg);

  /* --------------------- RED_DRV (P1_1) --------------------- */
  port_pin_config_t port1_1_cfg = {
    kPORT_PullDown,           kPORT_LowPullResistor,  kPORT_FastSlewRate,        kPORT_PassiveFilterDisable,
    kPORT_OpenDrainDisable,   kPORT_LowDriveStrength, kPORT_NormalDriveStrength, kPORT_MuxAsGpio,
    kPORT_InputBufferDisable, kPORT_InputNormal,      kPORT_UnlockRegister};
  PORT_SetPinConfig(PORT1, RED_DRV_PIN, &port1_1_cfg);
  GPIO_PinInit(GPIO1, RED_DRV_PIN, &out_cfg);
}

/**
 * @brief Sets the state of the LED with the given color.
 *
 * @param color The color of the LED to set.
 * @param state The state of the LED to set.
 *
 * This function sets the state of the LED with the given color.
 * The state of the LED can be either LED_ON or LED_OFF.
 *
 * @example
 * bsp_led_set(LED_RED, LED_ON);
 * This example sets the state of the LED with the color RED to LED_ON.
 */
void
bsp_single_led_set(led_color_t color, led_state_t state) {
  switch (color) {
    case LED_RED:
      if (state == LED_ON) {
        RED_DRV_SET();
      } else {
        RED_DRV_CLR();
      }
      break;
    case LED_GREEN:
      if (state == LED_ON) {
        GREEN_DRV_SET();
      } else {
        GREEN_DRV_CLR();
      }
      break;
    case LED_BLUE:
      if (state == LED_ON) {
        BLUE_DRV_SET();
      } else {
        BLUE_DRV_CLR();
      }
      break;
    default: break;
  }
}
void
bsp_color_rgb_set(rgb_color_t color) {
  switch (color) {
    case RED:
        RED_DRV_SET();
        GREEN_DRV_CLR();
        BLUE_DRV_CLR();
      break;
    case GREEN:
        RED_DRV_CLR();
        GREEN_DRV_SET();
        BLUE_DRV_CLR();
      break;
    case BLUE:
        RED_DRV_CLR();
        GREEN_DRV_CLR();
        BLUE_DRV_SET();
      break;
    case WHITE:
        RED_DRV_SET();
        GREEN_DRV_SET();
        BLUE_DRV_SET();
      break;
    case YELLOW:
      RED_DRV_SET();
      GREEN_DRV_SET();
      BLUE_DRV_CLR();
    break;
    case AZURE:
      RED_DRV_CLR();
      GREEN_DRV_SET();
      BLUE_DRV_SET();
    break;
    case OFF:
        RED_DRV_CLR();
        GREEN_DRV_CLR();
        BLUE_DRV_CLR();
      break;
    default: break;
  }
}