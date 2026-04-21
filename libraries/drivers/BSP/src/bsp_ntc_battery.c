/******************************************************************************
* Filename              :   bsp_ntc_battery.c
* Author                :   Giulio Nardon
* Origin Date           :   20 April 2026
*
* Copyright (c) 2026 EAS Engineering srl. All rights reserved.  
*
******************************************************************************/

/** @file bsp_ntc_battery.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include "bsp_ntc_battery.h"

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

/*****************************************************************************
* Module Typedefs
******************************************************************************/

typedef struct {
  uint32_t adc_value_min;
  uint32_t adc_value_max;
  float m;
  float q;
} ntc_linearization_t;

/*****************************************************************************
* Function Prototypes
******************************************************************************/

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

static ntc_linearization_t ntc_linearization_table[] = {
  {
    0,
    1632,
    -0.0542f,
    140.7f,
  },
  {
    1633,
    2267,
    -0.0299f,
    107.53f,
  },
  {
    2268,
    2949,
    -0.0293f,
    106.44f,
  },
  {
    2950,
    3493,
    -0.0369f,
    129.58f,
  },
  {
    3494,
    4095,
    -0.08f,
    248.01f,
  },
};

/*****************************************************************************
* Function Definitions
******************************************************************************/

/**
 * @brief Calculate the temperature from the given ADC value and gain using
 *        the provided ntc_linearization_table.
 *
 * @param adc_value The raw ADC value from the ntc measurement.
 * @param max_value_adc The maximum value of the ADC.
 *
 * @return The calculated temperature in degree Celsius, or 0 if the ADC value
 *         is out of range.
 */
float
bsp_ntc_battery_get_temperature(uint32_t adc_value, uint32_t max_value_adc) {
  float gain = (float)(4095.0 / (float)max_value_adc);
  float value = (float)adc_value;
  value *= gain;

  for (uint32_t i = 0; i < sizeof(ntc_linearization_table) / sizeof(ntc_linearization_t); ++i) {
    if (value >= ntc_linearization_table[i].adc_value_min && value <= ntc_linearization_table[i].adc_value_max) {
      return ((ntc_linearization_table[i].m * value) + ntc_linearization_table[i].q);
    }
  }

  return 0;
}