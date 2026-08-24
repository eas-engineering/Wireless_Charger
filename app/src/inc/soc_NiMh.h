/*****************************************************************************
* Filename              :   soc_NiMh.h
* Author                :   Giulio Nardon
* Origin Date           :   26 March 2026
*
* Copyright (c) 2026 EAS Engineering srl. All rights reserved.
*
******************************************************************************/

/** @file soc.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef SOC_H_
#define SOC_H_

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
uint16_t soc_from_voltage_nimh(uint16_t v_mm, uint16_t tempC, uint16_t i_mm, bool isCharging);

uint16_t soc_coulomb(float Q_mAh, uint16_t Q_mAh_max,int16_t temp, bool first_cycle);

uint16_t soc_estimate(uint16_t soc_cc, uint16_t soc_v, uint16_t i_mm, bool isCharging, bool first_cycle);

uint16_t time_charge_estimate(float mAh, float mAh_max, uint16_t ibat, bool last_minutes, bool first_cycle, int16_t temp);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /*SOC_H_*/

/*** End of File *************************************************************/
