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
#include "soc_NiMh.h"

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
* Function Prototypes
******************************************************************************/

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*****************************************************************************
* Function Definitions
******************************************************************************/

float capacity_temp_corr(float tempC)
{
    if (tempC <= 0)       return 0.75f;   // 75%
    if (tempC <= 10)      return 0.85f;
    if (tempC <= 20)      return 0.95f;
    if (tempC <= 35)      return 1.00f;
    if (tempC <= 45)      return 0.98f;
    return 1.0f;   // safety
}

static float Q_mAh = 0.0f;          // carica accumulata
static float Qmax_mAh = 1900.0f;    // capacità variabile

uint8_t soc_coulomb_advanced(float I_mA, float tempC, float dt_sec)
{
    /* Aggiorna capacità max in funzione temperatura */
    Qmax_mAh = 1900.0f * capacity_temp_corr(tempC);

    /* Calcola contributo Coulomb counting */
    float dQ = I_mA * (dt_sec / 3600.0f);   // mA * h

    /* Autoscarica */
    //float dQ_auto = autoscarica_per_sample(tempC, dt_sec / 3600.0f, Qmax_mAh);

    /* Aggiorna carica interna */ 
    Q_mAh += dQ;
    //Q_mAh -= dQ_auto;

    /* Saturazione */ 
    if (Q_mAh < 0) Q_mAh = 0;
    if (Q_mAh > Qmax_mAh) Q_mAh = Qmax_mAh;

    /* SoC grezzo */ 
    return (uint8_t)((Q_mAh / Qmax_mAh) * 100.0f);
}

////////////////// stima SOC da tensione //////////////////////////////

/* ----------------------------------------------------------
   Correzione tensione in funzione della temperatura.
   Le Ni-MH abbassano la tensione a freddo ~1.5mV/°C
-----------------------------------------------------------*/
static float adjust_voltage_for_temp(float v, int16_t tempC) {
    // correzione verso 25°C
    float correction = (float)(tempC - 25) * 0.0015f;   // 1.5 mV/°C
    return v - correction;
}

/* ----------------------------------------------------------
   Stima SoC da tensione Ni-MH
   Basato sulle curve Panasonic (grafici scarica)
-----------------------------------------------------------*/
uint8_t soc_from_voltage_nimh(float vcell, int16_t tempC) {

    // 1) Correggi tensione per temperatura
    float v = adjust_voltage_for_temp(vcell, tempC);

    // 2) Limiti fisici estremi
    if (v <= 1.05f) return 0;     // completamente scarica
    if (v >= 1.36f) return 100;   // completamente carica

    float soc = 0.0f;

    // 3) Interpolazione per fasce (ricavate dai grafici Panasonic)
    if (v >= 1.32f) {          // 90–100%
        soc = 90.0f + (v - 1.32f) * (10.0f / 0.04f);
    }
    else if (v >= 1.30f) {     // 80–90%
        soc = 80.0f + (v - 1.30f) * (10.0f / 0.02f);
    }
    else if (v >= 1.28f) {     // 60–80%
        soc = 60.0f + (v - 1.28f) * (20.0f / 0.02f);
    }
    else if (v >= 1.24f) {     // 40–60%
        soc = 40.0f + (v - 1.24f) * (20.0f / 0.04f);
    }
    else if (v >= 1.20f) {     // 20–40%
        soc = 20.0f + (v - 1.20f) * (20.0f / 0.04f);
    }
    else {                     // 0–20%
        soc = (v - 1.05f) * (20.0f / 0.15f);
    }

    // 4) Clamp finale
    if (soc < 0) soc = 0;
    if (soc > 100) soc = 100;

    return (uint8_t)soc;
}

////////////////////////////////////////////////////////////////

typedef struct {
    float x;   // SoC
    float P;   // varianza
} kalman_soc_t;

static kalman_soc_t KF = { .x = 50.0f, .P = 10.0f };

float kalman_update(float soc_cc, float soc_voltage)
{
    const float Q = 0.01f;   // rumore processo
    const float R = 4.0f;    // rumore misura

    // Predict
    KF.x = soc_cc;
    KF.P = KF.P + Q;

    // Update
    float K = KF.P / (KF.P + R);
    KF.x = KF.x + K * (soc_voltage - KF.x);
    KF.P = (1 - K) * KF.P;

    // Saturazione
    if (KF.x < 0) KF.x = 0;
    if (KF.x > 100) KF.x = 100;

    return KF.x;
}

float filter_current(float I)
{
    static float If = 0.0f;
    const float alpha = 0.2f;   // smoothing
    If = If + alpha * (I - If);
    return If;
}

uint8_t soc_estimate(float vcell, float I_mA_raw, float tempC, float dt_sec)
{
    // 1) Filtra correnti impulsive
    float I_mA = filter_current(I_mA_raw);

    // 2) Coulomb Counting avanzato
    uint8_t soc_cc = soc_coulomb_advanced(I_mA, tempC, dt_sec);

    // 3) Voltage-based SoC
    uint8_t soc_v = soc_from_voltage_nimh(vcell, (int16_t)tempC);

    // 4) Kalman fusion
    float soc = kalman_update(soc_cc, soc_v);

    return (uint8_t)soc;
}


