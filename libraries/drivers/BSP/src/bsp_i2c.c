/******************************************************************************
 * Filename              : bsp_i2c.c
 * Author                : Giulio Dalla Vecchia
 * Origin Date           : 30 December 2025
 *
 * Copyright (c) 2025 EAS Engineering srl.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ******************************************************************************/

/** @file bsp_i2c.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include "bsp_i2c.h"
#include "fsl_common.h"
#include "fsl_lpi2c.h"
#include "fsl_port.h"

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/

#define I2C_BAUDRATE               100000U

#define I2C_MASTER_BASE            LPI2C0

#define I2C_MASTER_SLAVE_ADDR_7BIT 0x7EU

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

/*****************************************************************************
* Module Typedefs
******************************************************************************/

/*****************************************************************************
* Function Prototypes
******************************************************************************/

void bsp_i2c_init(void);
static void bsp_i2c_pin_init(void);
static void bsp_i2c_peripheral_init(void);

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/


/*****************************************************************************
* Function Definitions
******************************************************************************/

void bsp_i2c_init(){
  bsp_i2c_pin_init();
  bsp_i2c_peripheral_init();
}

i2c_error_t bsp_i2c_writeBytes(uint8_t deviceAddress,
                              i2c_mem_addr_t memAddrType,
                              uint16_t memAddress,
                              uint16_t numBytes,
                              uint8_t *pData)
{
    lpi2c_master_transfer_t xfer = {0};
    uint8_t addrBuf[2];

    if ((pData == NULL) || (numBytes == 0))
    {
        return I2C_ERROR;
    }

    /* Preparazione indirizzo memoria */
    if (memAddrType == I2C_MEM_ADDR_16)
    {
        addrBuf[0] = (uint8_t)((memAddress >> 8) & 0xFF);  // MSB
        addrBuf[1] = (uint8_t)(memAddress & 0xFF);         // LSB

        xfer.subaddress = ((uint32_t)addrBuf[0] << 8) | addrBuf[1];
        xfer.subaddressSize = 2;
    }
    else
    {
        xfer.subaddress = memAddress & 0xFF;
        xfer.subaddressSize = 1;
    }

    xfer.slaveAddress   = deviceAddress;
    xfer.direction      = kLPI2C_Write;
    xfer.data           = pData;
    xfer.dataSize       = numBytes;
    xfer.flags          = kLPI2C_TransferDefaultFlag;

    if (LPI2C_MasterTransferBlocking(I2C_MASTER_BASE, &xfer) != kStatus_Success)
    {
        return I2C_ERROR;
    }

    return I2C_NO_ERROR;
}

i2c_error_t bsp_i2c_readByte(uint8_t deviceAddress,
                             uint16_t memAddress,
                             uint16_t numBytes,
                             uint8_t *pData)
{
    lpi2c_master_transfer_t xfer = {0};

    if ((pData == NULL) || (numBytes == 0))
    {
        return I2C_ERROR;
    }

    /* Configurazione trasferimento I2C */
    xfer.slaveAddress     = deviceAddress;
    xfer.direction        = kLPI2C_Read;
    xfer.subaddress       = memAddress;
    xfer.subaddressSize   = 2;                 
    xfer.data             = pData;
    xfer.dataSize         = numBytes;
    xfer.flags            = kLPI2C_TransferDefaultFlag;
    
    status_t status;
    status = LPI2C_MasterTransferBlocking(I2C_MASTER_BASE, &xfer);
    if (status != kStatus_Success)
    {
        return I2C_ERROR;
    }

    return I2C_NO_ERROR;
}

i2c_error_t bsp_i2c_getState(i2c_state_t *pt_state)
{
    if (pt_state == NULL)
    {
        return I2C_ERROR;
    }

    /* Controllo Bus Busy Flag del modulo LPI2C */
    if ((I2C_MASTER_BASE->MSR & LPI2C_MSR_BBF_MASK) != 0U)
    {
        *pt_state = I2C_STATE_BUSY;
    }
    else
    {
        *pt_state = I2C_STATE_READY;
    }

    return I2C_NO_ERROR;
}

/**
 * @brief This function is used to initialize the board I2C pin settings.
 *
 * The function configures the PORT3_27 (pin 34) as LPI2C0_SCL and PORT3_28 (pin 33) as LPI2C0_SDA.
 *
 * The pin is configured as open drain output with low drive strength and without internal pull-up/down resistor.
 * The pin is also configured with fast slew rate and with passive input filter disabled.
 * The pin is configured as LPI2C0_SCL and LPI2C0_SDA with digital input enabled and not inverted.
 */
static void
bsp_i2c_pin_init(void) {

  /* Write to PORT3: Peripheral clock is enabled */
  CLOCK_EnableClock(kCLOCK_GatePORT3);

  /* PORT3 peripheral is released from reset */
  RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);

  const port_pin_config_t port3_27_pin17_config = {/* Internal pull-up resistor is enabled */
                                                   kPORT_PullUp,
                                                   /* Low internal pull resistor value is selected. */
                                                   kPORT_LowPullResistor,
                                                   /* Fast slew rate is configured */
                                                   kPORT_FastSlewRate,
                                                   /* Passive input filter is disabled */
                                                   kPORT_PassiveFilterDisable,
                                                   /* Open drain output is enabled */
                                                   kPORT_OpenDrainEnable,
                                                   /* Low drive strength is configured */
                                                   kPORT_LowDriveStrength,
                                                   /* Normal drive strength is configured */
                                                   kPORT_NormalDriveStrength,
                                                   /* Pin is configured as LPI2C0_SCL */
                                                   kPORT_MuxAlt2,
                                                   /* Digital input enabled */
                                                   kPORT_InputBufferEnable,
                                                   /* Digital input is not inverted */
                                                   kPORT_InputNormal,
                                                   /* Pin Control Register fields [15:0] are not locked */
                                                   kPORT_UnlockRegister};
  /* PORT3_27 (pin 17) is configured as LPI2C0_SCL */
  PORT_SetPinConfig(PORT3, 27U, &port3_27_pin17_config);

  const port_pin_config_t port3_28_pin16_config = {/* Internal pull-up resistor is enabled */
                                                   kPORT_PullUp,
                                                   /* Low internal pull resistor value is selected. */
                                                   kPORT_LowPullResistor,
                                                   /* Fast slew rate is configured */
                                                   kPORT_FastSlewRate,
                                                   /* Passive input filter is disabled */
                                                   kPORT_PassiveFilterDisable,
                                                   /* Open drain output is enabled */
                                                   kPORT_OpenDrainEnable,
                                                   /* Low drive strength is configured */
                                                   kPORT_LowDriveStrength,
                                                   /* Normal drive strength is configured */
                                                   kPORT_NormalDriveStrength,
                                                   /* Pin is configured as LPI2C0_SDA */
                                                   kPORT_MuxAlt2,
                                                   /* Digital input enabled */
                                                   kPORT_InputBufferEnable,
                                                   /* Digital input is not inverted */
                                                   kPORT_InputNormal,
                                                   /* Pin Control Register fields [15:0] are not locked */
                                                   kPORT_UnlockRegister};
  /* PORT3_28 (pin 33) is configured as LPI2C0_SDA */
  PORT_SetPinConfig(PORT3, 28U, &port3_28_pin16_config);
}

/**
 * @brief Initialize the LPI2C peripheral.
 *
 * This function initializes the LPI2C0 peripheral by attaching the clock, releasing the peripheral
 * from reset, changing the default baudrate configuration, and initializing the LPI2C master peripheral.
 *
 * @note This function is called by the BSP initialization function.
 */
static void
bsp_i2c_peripheral_init(void) {

  lpi2c_master_config_t masterConfig;

  /* Attach peripheral clock */
  CLOCK_SetClockDiv(kCLOCK_DivLPI2C0, 1u);
  CLOCK_AttachClk(kFRO12M_to_LPI2C0);

  /* LPI2C0 peripheral is released from reset */
  RESET_ReleasePeripheralReset(kLPI2C0_RST_SHIFT_RSTn);

  LPI2C_MasterGetDefaultConfig(&masterConfig);

  /* Change the default baudrate configuration */
  masterConfig.baudRate_Hz = I2C_BAUDRATE;

  /* Initialize the LPI2C master peripheral */
  LPI2C_MasterInit(I2C_MASTER_BASE, &masterConfig, CLOCK_GetLpi2cClkFreq());
}
