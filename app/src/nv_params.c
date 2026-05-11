/******************************************************************************
* Filename              :   nv_params.c
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   20 April 2026
*
* Copyright (c) 2025 EAS Engineering srl. All rights reserved.  
* SPDX-License-Identifier: MIT
*
******************************************************************************/

/** @file nv_params.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include "nv_params.h"
#include "bsp_eeprom.h"

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/

/* =========================================================
 *  Packing portability
 * ========================================================= */
#if defined(__GNUC__) || defined(__clang__)
#define NV_PACKED_ATTR __attribute__((packed))
#else
#define NV_PACKED_ATTR
#endif

/* =========================================================
 *  Record format: 6 bytes
 * ========================================================= */
#define NV_MAGIC       ((uint16_t)0xE071u)

#define NV_RECORD_SIZE ((uint16_t)sizeof(nv_record_t))

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

/*****************************************************************************
* Module Typedefs
******************************************************************************/

typedef struct {
  uint16_t magic;
  uint16_t value;
  uint16_t crc;
} NV_PACKED_ATTR nv_record_t;

/* Compile-time record size check (C11). If not available, you can remove it. */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Static_assert(sizeof(nv_record_t) == 6u, "nv_record_t must be 6 bytes");
#endif

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
 * @brief CRC16-CCITT (FALSE) computation
 *
 * This function computes the CRC16-CCITT (FALSE) checksum of a given data block.
 * The CRC16-CCITT (FALSE) algorithm is defined as:
 *   - polynomial: 0x1021
 *   - initial value: 0xFFFF
 * The function takes a pointer to a data block and its length as input, and
 * returns the computed CRC16-CCITT (FALSE) checksum.
 *
 * @param data Pointer to the data block
 * @param len Length of the data block
 * @return Computed CRC16-CCITT (FALSE) checksum
 */
static uint16_t
crc16_ccitt_false(const uint8_t* data, uint16_t len) {
  uint16_t crc = 0xFFFFu;
  for (uint16_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t b = 0; b < 8; b++) {
      if (crc & 0x8000u) {
        crc = (uint16_t)((crc << 1) ^ 0x1021u);
      } else {
        crc = (uint16_t)(crc << 1);
      }
    }
  }
  return crc;
}

/**
 * @brief Calculate CRC16-CCITT (FALSE) checksum of packed NV record attribute.
 *
 * @param magic Magic value of the packed NV record attribute.
 * @param device_tag Device tag of the packed NV record attribute.
 * @param id ID of the packed NV record attribute.
 * @param value Value of the packed NV record attribute.
 * @return Calculated CRC16-CCITT (FALSE) checksum.
 */
static uint16_t
calc_crc(uint16_t magic, uint16_t device_tag, uint16_t id, uint16_t value) {
  uint8_t buf[8];
  buf[0] = (uint8_t)(magic & 0xFFu);
  buf[1] = (uint8_t)(magic >> 8);
  buf[2] = (uint8_t)(device_tag & 0xFFu);
  buf[3] = (uint8_t)(device_tag >> 8);
  buf[4] = (uint8_t)(id & 0xFFu);
  buf[5] = (uint8_t)(id >> 8);
  buf[6] = (uint8_t)(value & 0xFFu);
  buf[7] = (uint8_t)(value >> 8);
  return crc16_ccitt_false(buf, (uint16_t)sizeof(buf));
}

/**
 * @brief Check if the given nv_params_ctx_t* is valid.
 *
 * A valid context must have a non-null pointer to the descriptor array.
 *
 * @param ctx Pointer to the nv_params_ctx_t to be checked.
 * @return true if the context is valid, false otherwise.
 */
static bool
ctx_ok(const nv_params_ctx_t* ctx) {
  return (ctx && ctx->desc);
}

/**
 * @brief Check if the given parameter ID is valid for the given context.
 *
 * @param ctx Pointer to the nv_params_ctx_t to be checked.
 * @param id Parameter ID to be checked.
 * @return true if the parameter ID is valid for the given context, false otherwise.
 */
static bool
id_valid(const nv_params_ctx_t* ctx, uint16_t id) {
  return ctx_ok(ctx) && (id < ctx->count);
}

/**
 * @brief Return the EEPROM address where the record of parameter 'id' starts.
 *
 * This function takes a pointer to a nv_params_ctx_t and a parameter ID as input, and
 * returns the EEPROM address where the record of the parameter starts.
 *
 * @param ctx Pointer to the nv_params_ctx_t to be used.
 * @param id Parameter ID whose record address is to be retrieved.
 * @return EEPROM address where the record of parameter 'id' starts.
 */
static uint16_t
addr_of(const nv_params_ctx_t* ctx, uint16_t id) {
  return (uint16_t)((uint16_t)(id * NV_RECORD_SIZE));
}

/**
 * @brief Check if a given parameter value is within the valid range.
 *
 * @param ctx Pointer to the nv_params_ctx_t to be used.
 * @param id Parameter ID whose range is to be checked.
 * @param v Value to be checked.
 * @return true if the value is within the valid range, false otherwise.
 */
static bool
in_range(const nv_params_ctx_t* ctx, uint16_t id, uint16_t v) {
  return (v >= ctx->desc[id].minv) && (v <= ctx->desc[id].maxv);
}

/**
 * @brief Read parameter value.
 * @param ctx Pointer to the nv_params_ctx_t to be used.
 * @param id Parameter ID whose value is to be retrieved.
 * @return stored value if valid (magic+crc+range), otherwise default.
 */
uint16_t
nv_param_get(const nv_params_ctx_t* ctx, uint16_t id) {
  if (!id_valid(ctx, id)) {
    return 0u;
  }

  const uint16_t defv = ctx->desc[id].defv;
  const uint16_t addr = addr_of(ctx, id);

  nv_record_t rec;
  if (bsp_eeprom_readByte(addr,NV_RECORD_SIZE,(uint8_t *)&rec) != EEPROM_NO_ERROR) {
    return defv;
  }

  if (rec.magic != NV_MAGIC) {
    return defv;
  }

  const uint16_t expected = calc_crc(rec.magic, ctx->device_tag, id, rec.value);
  if (rec.crc != expected) {
    return defv;
  }

  if (!in_range(ctx, id, rec.value)) {
    return defv;
  }

  return rec.value;
}

/**
 * @brief Write parameter value with magic+crc.
 *        If outside range and reject policy enabled -> returns false, no write.
 *
 * @param ctx Pointer to the nv_params_ctx_t to be used.
 * @param id Parameter ID whose value is to be written.
 * @param value Value to be written.
 * @return true if the write was successful, false otherwise.
 */
bool
nv_param_set(const nv_params_ctx_t* ctx, uint16_t id, uint16_t value) {
  if (!id_valid(ctx, id)) {
    return false;
  }

  /* If ever used in future, you could allow other policies here */
  if (!in_range(ctx, id, value)) {
    return false;
  }

  const uint16_t addr = addr_of(ctx, id);

  nv_record_t rec;
  rec.magic = NV_MAGIC;
  rec.value = value;
  rec.crc = calc_crc(rec.magic, ctx->device_tag, id, rec.value);

  return bsp_eeprom_writeBytes(addr,NV_RECORD_SIZE,(uint8_t *)&rec);
}

/**uint8_yt
 * @brief Reset one parameter to its default (writes it).
 *
 * @param ctx Pointer to the nv_params_ctx_t to be used.
 * @param id Parameter ID whose value is to be reset to default.
 * @return true if the write was successful, false otherwise.
 */
bool
nv_param_reset_default(const nv_params_ctx_t* ctx, uint16_t id) {
  if (!id_valid(ctx, id)) {
    return false;
  }
  return nv_param_set(ctx, id, ctx->desc[id].defv);
}

/**
 * @brief Reset all parameters to defaults (writes all records).
 *        Optional utility.
 *
 * @param ctx Pointer to the nv_params_ctx_t to be used.
 * @return true if all parameters were reset successfully, false otherwise.
 */
bool
nv_params_reset_all_defaults(const nv_params_ctx_t* ctx) {
  if (!ctx_ok(ctx)) {
    return false;
  }

  bool ok = true;
  for (uint16_t i = 0; i < ctx->count; i++) {
    ok &= nv_param_set(ctx, i, ctx->desc[i].defv);
  }
  return ok;
}

/**
 * @brief Return the EEPROM address where the record of parameter 'id' starts.
 *
 * @param ctx Pointer to the nv_params_ctx_t to be used.
 * @param id Parameter ID whose record address is to be retrieved.
 * @return EEPROM address where the record of parameter 'id' starts.
 */
uint16_t
nv_param_eeprom_addr(const nv_params_ctx_t* ctx, uint16_t id) {
  if (!id_valid(ctx, id)) {
    return 0u;
  }
  return addr_of(ctx, id);
}

/**
 * @brief Return total footprint in EEPROM used by the parameter table.
 *
 * @param ctx Pointer to the nv_params_ctx_t to be used.
 * @return total footprint in EEPROM used by the parameter table (in bytes).
 *
 * This function takes a pointer to a nv_params_ctx_t and returns the total
 * footprint in EEPROM used by the parameter table associated with the given
 * context.
 */
uint16_t
nv_params_footprint(const nv_params_ctx_t* ctx) {
  if (!ctx_ok(ctx)) {
    return 0u;
  }
  return (uint16_t)(ctx->count * NV_RECORD_SIZE);
}
