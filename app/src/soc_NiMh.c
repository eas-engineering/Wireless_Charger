/******************************************************************************
* Filename              :   soc_NiMh.c
* Author                :   Giulio Nardon
* Origin Date           :   26 March 2026
*
* Copyright (c) 2026 EAS Engineering srl. All rights reserved.  
*
******************************************************************************/

/** @file soc.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "soc_NiMh.h"

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/
#define N_CELLS  12
#define LUT_SIZE 15

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

/*****************************************************************************
* Module Typedefs
******************************************************************************/

typedef struct {
  float voltage; // Volt
  float soc;     // 0–1
} SocLutEntry;

/*****************************************************************************
* Function Prototypes
******************************************************************************/
static float adjust_voltage_for_temp(float v, uint16_t tempC);

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

static SocLutEntry soc_lut_on_charge[LUT_SIZE] = {{17.16f, 100.0f}, {17.05f, 95.0f}, {16.85f, 90.0f}, {16.62f, 80.0f},
                                                  {16.30f, 70.0f},  {16.09f, 60.0f}, {15.77f, 50.0f}, {15.55f, 45.0f},
                                                  {15.44f, 40.0f},  {15.34f, 35.0f}, {15.12f, 30.0f}, {14.91f, 20.0f},
                                                  {14.69f, 15.0f},  {14.48f, 10.0f}, {14.05f, 5.0f}};

static SocLutEntry soc_lut_on_work[LUT_SIZE] = {{16.8f, 100.0f}, {16.32f, 95.0f}, {16.08f, 90.0f}, {15.84f, 80.0f},
                                                {15.60f, 70.0f}, {15.36f, 60.0f}, {15.12f, 50.0f}, {14.88f, 40.0f},
                                                {14.64f, 30.0f}, {14.40f, 20.0f}, {14.16f, 15.0f}, {13.80f, 10.0f},
                                                {13.44f, 5.0f},  {13.20f, 2.0f},  {12.96f, 0.0f}};

/*****************************************************************************
* Function Definitions
******************************************************************************/

// float
// capacity_temp_corr(float tempC) {
//   if (tempC <= 0) {
//     return 0.75f;
//   }
//   if (tempC <= 10) {
//     return 0.85f;
//   }
//   if (tempC <= 20) {
//     return 0.95f;
//   }
//   if (tempC <= 35) {
//     return 1.00f;
//   }
//   if (tempC <= 45) {
//     return 0.98f;
//   }
//   return 1.0f;
// }
float
capacity_temp_corr(float tempC) {
  if (tempC <= 0) {
    return 0.75f;
  }
  if (tempC <= 10) {
    return 0.85f;
  }
  if (tempC <= 20) {
    return 0.95f;
  }
  if (tempC <= 29) {
    return 1.00f;
  }
  if (tempC <= 45) {
    return 0.98f;
  }
  return 1.0f;
}

uint16_t
soc_coulomb(float Q_mAh, uint16_t Q_mAh_max, int16_t temp, bool first_cycle) {

  if (first_cycle) {
    return 0;
  }
  float tempC = temp / 100.0f;
  float Qmax_mAh = (float)Q_mAh_max * capacity_temp_corr(tempC);
  uint8_t soc;
  /* Saturazione */
  if (Q_mAh < 0) {
    Q_mAh = 0;
  }
  if (Q_mAh > Qmax_mAh) {
    Q_mAh = Qmax_mAh;
  }
  soc = (Q_mAh / Qmax_mAh) * 100.0f;
  soc = (uint16_t)((soc + 2.5f) / 5.0f) * 5;
  /* SoC */
  // if(soc > 95 && isCharging) {
  //   soc = 95;
  // }
  return (uint8_t)(soc);
}

float
filter_voltage(float v_raw_f) {
  static float vf = 0.0f;
  /* fattore di smorzamento */
  const float alpha = 0.8f;
  /* filtro passa basso primo ordine */
  vf = vf + alpha * (v_raw_f - vf);
  return vf;
}

uint16_t
soc_from_voltage_nimh(uint16_t v_mm, uint16_t tempC, uint16_t i_mm, bool isCharging) {
/* --- parametri --- */
#define IDLE_CURRENT_MA 250U
#define ALPHA_VOLTAGE   0.5f // filtro IIR tensione
#define MAX_SOC_STEP    1.0f // max variazione SoC [%] per chiamata

  static float soc_filt = 0.0f;
  static float v_filt = 0.0f;
  static bool init = false;

  /* --- Se non idle, NON aggiornare SoC --- */
  if (i_mm > IDLE_CURRENT_MA && !isCharging) {
    return (uint16_t)(soc_filt);
  }

  /* --- Selezione LUT --- */
  const SocLutEntry* lut = isCharging ? soc_lut_on_charge : soc_lut_on_work;

  /* --- Conversione e filtro tensione --- */
  float v = v_mm / 100.0f; // 1230 -> 12.30V

  if (!init) {
    v_filt = v;
    soc_filt = lut[LUT_SIZE - 1].soc; // fallback iniziale
    //init = true;
  } else {
    /* filtro passa-basso */
    v_filt += ALPHA_VOLTAGE * (v - v_filt);
  }

  /* --- Correzione temperatura --- */
  v = adjust_voltage_for_temp(v_filt, tempC);

  /* --- Clamp estremi LUT --- */
  if (v >= lut[0].voltage) {
    soc_filt = lut[0].soc;
    return (uint16_t)(soc_filt);
  }

  if (v <= lut[LUT_SIZE - 1].voltage) {
    soc_filt = lut[LUT_SIZE - 1].soc;
    return (uint16_t)(soc_filt);
  }

  /* --- Interpolazione lineare --- */
  float soc_new = soc_filt;

  for (int i = 0; i < LUT_SIZE - 1; i++) {
    if (v <= lut[i].voltage && v > lut[i + 1].voltage) {

      float v1 = lut[i].voltage;
      float v2 = lut[i + 1].voltage;
      float s1 = lut[i].soc;
      float s2 = lut[i + 1].soc;

      float t = (v - v2) / (v1 - v2);
      soc_new = s2 + t * (s1 - s2);
      break;
    }
  }

  /* --- Limitazione velocità variazione SoC --- */

  if (!init) {
    soc_filt = soc_new;
    init = true;
  } else {
    float delta = soc_new - soc_filt;

    if (delta > MAX_SOC_STEP) {
      delta = MAX_SOC_STEP;
    }
    if (delta < -MAX_SOC_STEP) {
      delta = -MAX_SOC_STEP;
    }

    soc_filt += delta;
  }

  return (uint16_t)(soc_filt);
}

/* ----------------------------------------------------------
   Correzione tensione in funzione della temperatura.
   Le Ni-MH abbassano la tensione a freddo ~1.5mV/°C
-----------------------------------------------------------*/
static float
adjust_voltage_for_temp(float v, uint16_t tempC) {
  /* correzione verso 25°C */
  float tempC_f = tempC / 100.0f;
  float correction = (float)(tempC_f - 25) * 0.0015f; // 1.5 mV/°C -> da valutare se è corretto
  return v - correction;
}

uint16_t
filter_current(uint16_t I) {
  float I_raw_f = I / 1.0f;
  static float If = 0.0f;
  /* fattore di smorzamento */
  const float alpha = 0.2f;
  /* filtro passa basso primo ordine */
  If = If + alpha * (I_raw_f - If);
  return (uint16_t)If;
}

uint16_t
soc_estimate(uint16_t soc_cc, uint16_t soc_v, uint16_t i_mm, bool isCharging, bool first_cycle) {

#define K_CORR        0.02f // forza correzione
#define MAX_CORR_STEP 0.5f  // max % per chiamata

  static float soc_cc_f = 0.0f;
  static bool init = false;

  /* se non ho mai raggiunto una ricarica completa, non posso affidarmi al coulomb counting */
  if (first_cycle) {
    soc_cc_f = soc_v;
    soc_cc = soc_v;
    return (uint16_t)(soc_v);
  }

  /* --- Clamp corrente --- */

  if (!init) {
    soc_cc_f = soc_cc;
    init = true;
  }

  /* --- Sempre clamp --- */
  if (soc_cc_f > 100.0f) {
    soc_cc_f = 100.0f;
  }
  if (soc_cc_f < 0.0f) {
    soc_cc_f = 0.0f;
  }
  /* --- Batteria idle → correggi lentamente --- */
  float err = (float)soc_v - soc_cc_f;
  float corr = err * K_CORR;

  if (corr > MAX_CORR_STEP) {
    corr = MAX_CORR_STEP;
  }
  if (corr < -MAX_CORR_STEP) {
    corr = -MAX_CORR_STEP;
  }

  if (!init) {
    soc_cc_f = soc_cc;
    init = true;
  } else {
    soc_cc_f += corr;

    /* --- Clamp finale --- */
    if (soc_cc_f > 100.0f) {
      soc_cc_f = 100.0f;
    }
    if (soc_cc_f < 0.0f) {
      soc_cc_f = 0.0f;
    }
  }

  return (uint16_t)(soc_cc_f);
}

uint16_t
time_charge_estimate(float mAh, float mAh_max, uint16_t ibat, bool last_minutes, bool first_cycle, int16_t temp) {

  if (first_cycle) {
    return 0;
  }
  /* se non ho ancora fatto il primo ciclo di ricarica, per il tempo rimanenete mi affido al SoC TODO !!!!!!*/
  static uint16_t minutes = 1000;
  float tempC = temp / 100.0f;
  float Qmax_mAh = (float)mAh_max * capacity_temp_corr(tempC);
  float Q_remaining = Qmax_mAh - mAh;
  float time_h;

  time_h = Q_remaining / (float)ibat;

  if (last_minutes) {
    /* se sono in CV impongo i minuti per la fine carica e inoltro quelli */
    return 5U;
  }

  uint16_t estimated = (uint16_t)(time_h * 60.0f + 5.0f);

  /* minimo storico */
  minutes = (estimated < minutes) ? estimated : minutes;

  /* arrotondamento a step di 5 minuti */
  minutes = ((minutes + 2) / 5) * 5;
  /* così segna 5 quando arriva in CV*/
  if (minutes < 10) {
    minutes = 10;
  }
  //minutes = (uint16_t)(time_h * 60.0f + 5.0f) < minutes ? (uint16_t)(time_h * 60.0f + 5.0f) : minutes;
  return minutes; // minuti
}