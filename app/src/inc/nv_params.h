/*****************************************************************************
* Filename              :   nv_params.h
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   20 April 2026
*
* Copyright (c) 2025 EAS Engineering srl. All rights reserved.
* SPDX-License-Identifier: MIT
*
******************************************************************************/

/** @file nv_params.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef NV_PARAMS_H_
#define NV_PARAMS_H_

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdbool.h>
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

/* =========================================================
 *  Parameter descriptor (compile-time per device)
 * ========================================================= */
typedef struct {
  uint16_t defv;
  uint16_t minv;
  uint16_t maxv;
} nv_param_desc_t;

/* =========================================================
 *  Context per device
 * ========================================================= */
typedef struct {
  const nv_param_desc_t* desc; /* array in FLASH/ROM */
  uint16_t count;              /* number of params */
  uint16_t device_tag;         /* unique tag to separate devices (eOTI/eWTI/...) */
} nv_params_ctx_t;

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*****************************************************************************
* Function Prototypes
******************************************************************************/

/**
 * @brief Read parameter value.
 * @return stored value if valid (magic+crc+range), otherwise default.
 */
uint16_t nv_param_get(const nv_params_ctx_t* ctx, uint16_t id);

/**
 * @brief Write parameter value with magic+crc.
 *        If outside range and reject policy enabled -> returns false, no write.
 */
bool nv_param_set(const nv_params_ctx_t* ctx, uint16_t id, uint16_t value);

/**
 * @brief Reset one parameter to its default (writes it).
 */
bool nv_param_reset_default(const nv_params_ctx_t* ctx, uint16_t id);

/**
 * @brief Reset all parameters to defaults (writes all records).
 *        Optional utility.
 */
bool nv_params_reset_all_defaults(const nv_params_ctx_t* ctx);

/**
 * @brief Return the EEPROM address where the record of parameter 'id' starts.
 */
uint16_t nv_param_eeprom_addr(const nv_params_ctx_t* ctx, uint16_t id);

/**
 * @brief Return total footprint in EEPROM used by the parameter table.
 */
uint16_t nv_params_footprint(const nv_params_ctx_t* ctx);

/**
 * }
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*NV_PARAMS_H_*/

/*** End of File *************************************************************/
