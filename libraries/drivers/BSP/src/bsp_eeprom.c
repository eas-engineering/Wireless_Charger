/**
******************************************************************************
* @file    	bsp_eeprom.c  
* @author  	Giulio Dalla Vecchia
* @version 	v1.0.0
* @date     23/05/2019
* @brief   	\n
* @details  \n
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "lwassert.h"

//#include "bsp_cfg.h"
#include "bsp_eeprom.h"
#include "bsp_eeprom_cfg.h"
#include "bsp_i2c.h"
#include "fsl_common.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

static eeprom_error_t bsp_eeprom_getState(eeprom_state_t* pt_state);

/* Private variables ---------------------------------------------------------*/

static uint8_t pui8_Buff[PAGE_WR_BUFF_SIZE_BYTE];
static uint32_t delay_in_us = 5000;

/** 
******************************************************************************
* @fn			  			bsp_eeprom_init(void)
* @brief					
* @param[in]      \n  
* @param[out]     \n
* @param[in,out]  \n
* @retval  		  	eeprom_error_t
*
* @details		  \n
******************************************************************************	
*/
eeprom_error_t
bsp_eeprom_init(void) {
  eeprom_error_t error = EEPROM_NO_ERROR;

  /* Do nothing else, i2c is already config */

  return error;
}

/** 
******************************************************************************
* @fn			  			bsp_eeprom_writeBytes(uint16_t ui16_MemAddress, 
																		    uint32_t ui32_NumOfByte, 
																		    uint8_t *pui8_DataTo)
* @brief					
* @param[in]      \n  
* @param[out]     \n
* @param[in,out]  \n
* @retval  		  	eeprom_error_t
*
* @details		  \n
******************************************************************************	
*/
eeprom_error_t
bsp_eeprom_writeBytes(uint16_t ui16_MemAddress, uint32_t ui32_NumOfBytes, uint8_t* pui8_DataTo) {
  eeprom_error_t error = EEPROM_NO_ERROR;
  eeprom_state_t state;
  uint16_t ui16_WriteStartAdd = 0;
  uint16_t ui16_RemBytes = 0;
  uint16_t ui16_BTW = 0;
  uint8_t* pui8_DataAdd = 0;
  uint16_t ui16_CurrPageStartAdd = 0;

  if (bsp_eeprom_getState(&state) != EEPROM_ERROR) {
    if (state == EEPROM_STATE_READY) {
      /*check if given addresses are withing the memory boundary*/
      if ((ui16_MemAddress + ui32_NumOfBytes) > (MEM_SIZE_BYTE - 1)) {
        error = EEPROM_ERROR;
      } else {
        ui16_WriteStartAdd = ui16_MemAddress;
        ui16_RemBytes = ui32_NumOfBytes;
        pui8_DataAdd = pui8_DataTo;

        /*cycle through all the bytes to write, maximum chunk size is 
          PAGE_WR_BUFF_SIZE_BYTE*/
        do {
          ui16_CurrPageStartAdd = (ui16_WriteStartAdd / PAGE_WR_BUFF_SIZE_BYTE) * PAGE_WR_BUFF_SIZE_BYTE;

          if ((ui16_WriteStartAdd + ui16_RemBytes) <= (ui16_CurrPageStartAdd + PAGE_WR_BUFF_SIZE_BYTE)) {
            ui16_BTW = ui16_RemBytes;
          } else {
            ui16_BTW = ui16_CurrPageStartAdd + PAGE_WR_BUFF_SIZE_BYTE - ui16_WriteStartAdd;
          }

          if (bsp_i2c_writeBytes(EEPROM_ADDRESS, I2C_MEM_ADDR_16, ui16_WriteStartAdd, ui16_BTW, pui8_DataAdd)
              != I2C_NO_ERROR) {
            error = EEPROM_ERROR;
          } else {
            ui16_RemBytes -= ui16_BTW;
            ui16_WriteStartAdd += ui16_BTW;
            pui8_DataAdd += ui16_BTW;
            SDK_DelayAtLeastUs(delay_in_us, SystemCoreClock);
          }
        } while ((ui16_RemBytes > 0) && (error == EEPROM_NO_ERROR));
      }
    } else {
      /* Eeprom busy */
      error = EEPROM_ERROR;
    }
  } else {
    error = EEPROM_ERROR;
  }

  return error;
}

/** 
******************************************************************************
* @fn			  			bsp_eeprom_readByte(uint16_t ui16_MemAddress, 
																			uint32_t ui32_NumOfByte,
																			uint8_t *pui8_DataFrom)
* @brief					
* @param[in]      \n  
* @param[out]     \n
* @param[in,out]  \n
* @retval  		  	eeprom_error_t
*
* @details		  \n
******************************************************************************	
*/
eeprom_error_t
bsp_eeprom_readByte(uint16_t ui16_MemAddress, uint32_t ui32_NumOfByte, uint8_t* pui8_DataFrom) {
  eeprom_error_t error = EEPROM_NO_ERROR;
  eeprom_state_t state;

  if (bsp_eeprom_getState(&state) != EEPROM_ERROR) {
    if (state == EEPROM_STATE_READY) {
      if (bsp_i2c_readByte(EEPROM_ADDRESS, ui16_MemAddress, ui32_NumOfByte, pui8_DataFrom) != I2C_NO_ERROR) {
        error = EEPROM_ERROR;
      }
    } else {
      /* Eeprom busy */
      error = EEPROM_ERROR;
    }
  } else {
    error = EEPROM_ERROR;
  }

  return error;
}

/**
******************************************************************************
* @fni            eeprom_error_t bsp_eeprom_MemorySet(uint16_t ui16_MemAddress,
*                 uint32_t ui32_NumOfBytes,uint8_t ui8_FillVal)
* @brief          set a memory area with a defined value.
* @param[in]      ui16_MemAddress beginning of the memory are to fill.
* @param[in]      ui32_NumOfBytes number of bytes to write.
* @param[in]      ui8_FillVal fill value.
* @retval         EEPROM_NO_ERROR or EEPROM_ERROR
*
* @details         \n
*                  \n
******************************************************************************
*/
eeprom_error_t
bsp_eeprom_MemorySet(uint16_t ui16_MemAddress, uint32_t ui32_NumOfBytes, uint8_t ui8_FillVal) {
  eeprom_error_t error = EEPROM_NO_ERROR;
  uint16_t ui16_WriteStartAdd = 0;
  uint16_t ui16_RemBytes = 0;
  uint16_t ui16_BTW = 0;
  uint16_t ui16_CurrPageStartAdd = 0;

  memset(pui8_Buff, ui8_FillVal, sizeof(pui8_Buff));

  /*check if given addresses are withing the memory boundary*/
  if ((ui16_MemAddress + ui32_NumOfBytes) > MEM_SIZE_BYTE) {
    error = EEPROM_ERROR;
  } else {
    ui16_WriteStartAdd = ui16_MemAddress;
    ui16_RemBytes = ui32_NumOfBytes;

    /*cycle through all the bytes to write, maximum chunk size is 
      PAGE_WR_BUFF_SIZE_BYTE*/
    do {
      ui16_CurrPageStartAdd = (ui16_WriteStartAdd / PAGE_WR_BUFF_SIZE_BYTE) * PAGE_WR_BUFF_SIZE_BYTE;
      if ((ui16_WriteStartAdd + ui16_RemBytes) <= (ui16_CurrPageStartAdd + PAGE_WR_BUFF_SIZE_BYTE)) {
        ui16_BTW = ui16_RemBytes;
      } else {
        ui16_BTW = ui16_CurrPageStartAdd + PAGE_WR_BUFF_SIZE_BYTE - ui16_WriteStartAdd;
      }

      if (bsp_i2c_writeBytes(EEPROM_ADDRESS, I2C_MEM_ADDR_16, ui16_WriteStartAdd, ui16_BTW, pui8_Buff)
          != I2C_NO_ERROR) {
        error = EEPROM_ERROR;
      } else {
        ui16_RemBytes -= ui16_BTW;
        ui16_WriteStartAdd += ui16_BTW;
        SDK_DelayAtLeastUs(delay_in_us, SystemCoreClock);
      }
    } while ((ui16_RemBytes > 0) && (error == EEPROM_NO_ERROR));
  }

  return error;
}

/** 
******************************************************************************
* @fn			        bsp_eeprom_memory_size_get(void)
* @brief					
* @param[in]      \n  
* @param[out]     \n
* @param[in,out]  \n
* @retval  		    uint32_t
*
* @details		  \n
******************************************************************************	
*/
uint32_t
bsp_eeprom_memory_size_get(void) {
  return MEM_SIZE_BYTE;
}

/** 
******************************************************************************
* @fn			  			bsp_eeprom_getState(eeprom_state_t *pt_state)
* @brief					
* @param[in]      \n  
* @param[out]     \n
* @param[in,out]  \n
* @retval  		  	eeprom_error_t
*
* @details		  \n
******************************************************************************	
*/
static eeprom_error_t
bsp_eeprom_getState(eeprom_state_t* pt_state) {
  eeprom_error_t error = EEPROM_NO_ERROR;
  i2c_state_t state;

  //lwassert(pt_state != NULL, ASSERT_SEVERITY_MINOR);

  /* Before starting a new communication transfer, you need to check the current   
		state of the peripheral; if it’s busy you need to wait for the end of current
		transfer before starting a new one.
		For simplicity reasons, this example is just waiting till the end of the 
		transfer, but application may perform other tasks while transfer operation
		is ongoing. */

  if (pt_state != NULL) {
    if (bsp_i2c_getState(&state) != I2C_NO_ERROR) {
      return EEPROM_ERROR;
    }

    *pt_state = (state == I2C_STATE_READY ? EEPROM_STATE_READY : EEPROM_STATE_BUSY);
  } else {
    error = EEPROM_ERROR;
  }

  return error;
}
