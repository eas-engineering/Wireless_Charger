/**
******************************************************************************
* @file    bsp_eeprom.h
* @author  Giulio Dalla Vecchia
* @version v1.0.0
* @date    23/05/2019
* @brief   \n
******************************************************************************
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BSP_EEPROM_H__
#define BSP_EEPROM_H__

#ifdef __cplusplus
 extern "C" {
#endif

/** 
*  \addtogroup Libraries
*  \details
*  @{
*/

/** 
*  \addtogroup BSP
*  \details
*  @{
*/

/** 
*  \addtogroup Eeprom
*  \details
*  @{
*/

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
   
/* Public define -------------------------------------------------------------*/ 
   
/* Public enum ---------------------------------------------------------------*/   

/* Public typedef ------------------------------------------------------------*/

typedef enum
{
	EEPROM_NO_ERROR,
	EEPROM_ERROR,
}eeprom_error_t;

typedef enum
{
	EEPROM_STATE_BUSY,
	EEPROM_STATE_READY,
}eeprom_state_t;

/* Public function prototypes ------------------------------------------------*/

eeprom_error_t bsp_eeprom_init(void);

eeprom_error_t bsp_eeprom_writeBytes(uint16_t ui16_MemAddress, 
																		 uint32_t ui32_NumOfByte, 
																		 uint8_t *pui8_DataTo);

eeprom_error_t bsp_eeprom_readByte(uint16_t ui16_MemAddress, 
																	 uint32_t ui32_NumOfByte,
																	 uint8_t *pui8_DataFrom);

eeprom_error_t bsp_eeprom_MemorySet(uint16_t ui16_MemAddress,
                                    uint32_t ui32_NumOfBytes,
                                    uint8_t ui8_FillVal);

uint32_t bsp_eeprom_memory_size_get(void);

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /*BSP_EEPROM_H__*/
