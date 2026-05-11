/**
******************************************************************************
* @file    bsp_eeprom_cfg.h
* @author  Giulio Dalla Vecchia
* @version v1.0.0
* @date    23/05/2019
* @brief   \n
******************************************************************************
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BSP_EEPROM_CFG_H__
#define BSP_EEPROM_CFG_H__

#ifdef __cplusplus
 extern "C" {
#endif
 
#include "24XX32A.h"
 
/* Public define -------------------------------------------------------------*/ 

/**
 * @brief  Define for the specific Eeprom
 * 
 */
#define EEPROM_ADDRESS        			0x50 //0xA0

#ifdef __cplusplus
}
#endif

#endif /*BSP_EEPROM_CFG_H__*/
