/*****************************************************************************
* Filename              :   bsp_ntc_battery.h
* Author                :   Giulio Nardon
* Origin Date           :   20 April 2026
*
* Copyright (c) 2026 EAS Engineering srl. All rights reserved.
*
******************************************************************************/

/** @file bsp_ntc_battery.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef BSP_NTC_BATTERY_H_
#define BSP_NTC_BATTERY_H_

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup        Template
 * \brief           Template functions group
 * \{
 */

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

/*****************************************************************************
* Module Typedefs
******************************************************************************/

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*****************************************************************************
* Function Prototypes
******************************************************************************/

float bsp_ntc_battery_get_temperature(uint32_t adc_value, uint32_t max_value_adc);

/**
 * }
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*BSP_NTC_BATTERY_H_*/

/*** End of File *************************************************************/
