/*****************************************************************************
* Filename              :   eoti_nv_param.h
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   20 April 2026
*
* Copyright (c) 2025 EAS Engineering srl. All rights reserved.
* SPDX-License-Identifier: MIT
*
******************************************************************************/

/** @file eoti_nv_param.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef EOTI_NV_PARAM_H_
#define EOTI_NV_PARAM_H_

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdint.h>
#include "nv_params.h"

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

/* Enum IDs for eOTI */
typedef enum {
  MAH_PARAM = 0,
  MAH_N_CYCLES_CHARGE_PARAM,
  MAH_TOT_PARAM,
  N_CYCLES_PARAM,
  ZERO_CURR_VAL_PARAM,
  FIRST_CYCLE_PARAM,
  BATTERY_PARAM_COUNT,
} param_id_t;

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

extern const nv_params_ctx_t battery_nv_ctx; /* context instance for battery */

/*****************************************************************************
* Function Prototypes
******************************************************************************/

/**
 * }
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*EOTI_NV_PARAM_H_*/

/*** End of File *************************************************************/
