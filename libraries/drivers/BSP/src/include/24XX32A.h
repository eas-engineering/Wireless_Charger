/**
******************************************************************************
* @file    24XX32A.h 
* @author  Massimo Miotti
* @version v1.0.0
* @date    19/09/2019
* @brief   32 kb eeprom 24AA32A/24LC32A parameters.
******************************************************************************
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _24XX32A_H__
#define _24XX32A_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported define -----------------------------------------------------------*/
#define PAGE_WR_BUFF_SIZE_BYTE                   (32UL)
#define MEM_SIZE_BYTE                            (0xFFFUL)
#define PAGE_WRITE_MAX_TIME_MS                   (5UL)
#define DATA_READ_TIMEOUT_MS                     (50UL)
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /*24XX32A_H__ */
