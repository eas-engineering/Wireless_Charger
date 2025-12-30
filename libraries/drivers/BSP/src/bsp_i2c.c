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

#define MAX_RETRIES_DEFAULT        3U

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

/*****************************************************************************
* Module Typedefs
******************************************************************************/

typedef struct {

  /* Super class */
  QHsm super;

  /* Reference to the container active object */
  QActive* container;

  /* Reference to the event */
  QEvt const* EvtRef;

  /* Deferred event queue */
  QEQueue deferredEvtQueue;

  /* I2C interface reference */
  lpi2c_master_handle_t transfer_handle;
  lpi2c_master_transfer_t masterXfer;

  /* Number of attempts */
  uint32_t retry;

  /* State reference */
  QStateHandler reminder;

} bsp_i2c_t;

/*****************************************************************************
* Function Prototypes
******************************************************************************/

static QState bsp_i2c_initial(bsp_i2c_t* const me, void const* const par);
static QState bsp_i2c_idle(bsp_i2c_t* const me, QEvt const* const e);
static QState bsp_i2c_busy(bsp_i2c_t* const me, QEvt const* const e);
static QState bsp_i2c_trasmitting(bsp_i2c_t* const me, QEvt const* const e);
static QState bsp_i2c_receiving(bsp_i2c_t* const me, QEvt const* const e);

static void bsp_i2c_pin_init(void);
static void bsp_i2c_peripheral_init(void);

static void lpi2c_master_callback(LPI2C_Type* base, lpi2c_master_handle_t* handle, status_t status, void* userData);

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

static bsp_i2c_t bsp_i2c_instances = {0};

/*****************************************************************************
* Function Definitions
******************************************************************************/

QHsm*
bsp_i2c_init(QActive* const container) {
  static QEvt const* DeferEvtQueueSto[10];
  bsp_i2c_t* me = &bsp_i2c_instances;

  QS_OBJ_DICTIONARY(&bsp_i2c_instances);
  QS_FUN_DICTIONARY(&bsp_i2c_initial);
  QS_FUN_DICTIONARY(&bsp_i2c_idle);
  QS_FUN_DICTIONARY(&bsp_i2c_trasmitting);

  QS_SIG_DICTIONARY(EXT_BUS_SEND_SIG, (void*)0);
  QS_SIG_DICTIONARY(EXT_BUS_RECEIVE_SIG, (void*)0);
  QS_SIG_DICTIONARY(EXT_BUS_TX_CMPL_SIG, (void*)0);
  QS_SIG_DICTIONARY(EXT_BUS_RX_CMPL_SIG, (void*)0);
  QS_SIG_DICTIONARY(EXT_BUS_REQ_CMPL_SIG, (void*)0);

  me->container = container;

  /* Initialize the deferred event queue */
  QEQueue_init(&me->deferredEvtQueue, DeferEvtQueueSto, Q_DIM(DeferEvtQueueSto));

  QHsm_ctor(&(me->super), Q_STATE_CAST(&bsp_i2c_initial));

  return &(me->super);
}

/**
 * @brief Initial state handler for the bsp_ext_bus_t state machine.
 *
 * This function sets the initial state of the bsp_ext_bus_t state machine
 * to bsp_i2c_idle. It is called during the initialization process.
 *
 * @param me Pointer to the bsp_ext_bus_t object.
 * @param par Unused parameter.
 *
 * @return Q_TRAN to transition to the bsp_i2c_idle state.
 */
static QState
bsp_i2c_initial(bsp_i2c_t* const me, void const* const par) {
  (void)par; // unused parameter

  bsp_i2c_pin_init();
  bsp_i2c_peripheral_init();

  /* Create the LPI2C handle for the non-blocking transfer */
  LPI2C_MasterTransferCreateHandle(I2C_MASTER_BASE, &(me->transfer_handle), lpi2c_master_callback, (void*)me);

  return Q_TRAN(&bsp_i2c_idle);
}

/**
 * @brief The idle state of the bsp_ext_bus_t state machine.
 *
 * In this state, the bsp_ext_bus_t state machine waits for an
 * EXT_BUS_SEND_SIG event to be posted to it. When such an event is
 * received, the state machine transitions to the bsp_i2c_trasmitting
 * state to handle the transmission.
 *
 * @param[in] me Pointer to the bsp_ext_bus_t object.
 * @param[in] e Pointer to the event that triggered the transition
 *              to this state.
 *
 * @return QState value indicating the status of the transition.
 */
static QState
bsp_i2c_idle(bsp_i2c_t* const me, QEvt const* const e) {
  QState status_;
  switch (e->sig) {
    case Q_INIT_SIG: {
      QActive_recall((QActive*)(me->container), &me->deferredEvtQueue);
      status_ = Q_HANDLED();
      break;
    }

    case BSP_I2C_SEND_SIG: {
      me->retry = 0U;
      Q_NEW_REF(me->EvtRef, QEvt);
      status_ = Q_TRAN(&bsp_i2c_trasmitting);
      break;
    }

    case BSP_I2C_RECEIVE_SIG: {
      me->retry = 0U;
      Q_NEW_REF(me->EvtRef, QEvt);
      status_ = Q_TRAN(&bsp_i2c_receiving);
      break;
    }

    default: {
      status_ = Q_SUPER(&QHsm_top);
      break;
    }
  }
  return status_;
}

/**
 * @brief The busy state of the bsp_ext_bus_t state machine.
 *
 * In this state, the bsp_ext_bus_t state machine defers any new
 * EXT_BUS_SEND_SIG or EXT_BUS_RECEIVE_SIG events until the current
 * transmission/reception is completed. When the transmission/reception
 * is completed, the state machine transitions back to the ext_bus_idle
 * state. If a Q_EXIT_SIG event is received, the state machine also
 * transitions back to the ext_bus_idle state.
 *
 * @param[in] me Pointer to the bsp_ext_bus_t object.
 * @param[in] e Pointer to the event that triggered the transition
 *              to this state.
 *
 * @return QState value indicating the status of the transition.
 */
static QState
bsp_i2c_busy(bsp_i2c_t* const me, QEvt const* const e) {
  QState status_;
  switch (e->sig) {
    case Q_ENTRY_SIG: {
      status_ = Q_HANDLED();
      break;
    }

    case Q_EXIT_SIG: {
      Q_DELETE_REF(me->EvtRef);
      status_ = Q_HANDLED();
      break;
    }

    case BSP_I2C_RECEIVE_SIG:
    case BSP_I2C_SEND_SIG: {
      QActive_defer((QActive*)(me->container), &me->deferredEvtQueue, e);
      status_ = Q_HANDLED();
      break;
    }

    case BSP_I2C_TX_RX_ERROR_SIG: {

      /* Increment the number of retries */
      ++(me->retry);

      if (me->retry < MAX_RETRIES_DEFAULT) {
        /* Send a request to the drive manager to read parameters configuration */
        status_ = Q_TRAN(me->reminder);
      } else {
        static const QEvt evt = QEVT_INITIALIZER(BSP_I2C_TIMEOUT_SIG);
        QACTIVE_POST(me->container, &evt, 0U);
        status_ = Q_HANDLED();
      }

      break;
    }

    default: {
      status_ = Q_SUPER(&bsp_i2c_idle);
      break;
    }
  }
  return status_;
}

/**
 * @brief The transmitting state of the bsp_ext_bus_t state machine.
 *
 * In this state, the bsp_ext_bus_t state machine transmits the data
 * contained in the me->txEvtRef event. When the transmission is
 * completed, the state machine transitions back to the bsp_i2c_idle
 * state. If another EXT_BUS_SEND_SIG event is received while the
 * transmission is in progress, it is deferred until the transmission
 * is completed.
 *
 * @param[in] me Pointer to the bsp_ext_bus_t object.
 * @param[in] e Pointer to the event that triggered the transition
 *              to this state.
 *
 * @return QState value indicating the status of the transition.
 */
static QState
bsp_i2c_trasmitting(bsp_i2c_t* const me, QEvt const* const e) {
  QState status_;
  switch (e->sig) {
    case Q_ENTRY_SIG: {

      status_ = Q_HANDLED();
      break;
    }

    case Q_INIT_SIG: {

      me->reminder = Q_STATE_CAST(&bsp_i2c_trasmitting);

      /* subAddress = 0x01, data = g_master_txBuff - write to slave.
      start + slaveaddress(w) + subAddress + length of data buffer + data buffer + stop*/
      me->masterXfer.slaveAddress = I2C_MASTER_SLAVE_ADDR_7BIT;
      me->masterXfer.direction = kLPI2C_Write;
      me->masterXfer.subaddress = (uint32_t)((BspI2CEvt_t*)me->EvtRef)->DevAddress;
      me->masterXfer.subaddressSize = 1;
      me->masterXfer.data = ((BspI2CEvt_t*)me->EvtRef)->pui8_data;
      me->masterXfer.dataSize = ((BspI2CEvt_t*)me->EvtRef)->len;
      me->masterXfer.flags = kLPI2C_TransferDefaultFlag;

      /* Send master non-blocking data to slave */
      if (LPI2C_MasterTransferNonBlocking(I2C_MASTER_BASE, &(me->transfer_handle), &(me->masterXfer))
          != kStatus_Success) {
        QEvt* const evt = (QEvt* const)(e);
        evt->sig = BSP_I2C_REQ_ERROR_SIG;
        QACTIVE_POST(((BspI2CEvt_t*)me->EvtRef)->AO_sender, evt, (QActive*)me);
        status_ = Q_TRAN(&bsp_i2c_idle);
      } else {
        status_ = Q_HANDLED();
      }

      break;
    }

    case BSP_I2C_TX_RX_CMPL_SIG: {

      QEvt* const evt = (QEvt* const)(me->EvtRef);

      /* When It's done, go back to idle */
      evt->sig = BSP_I2C_REQ_CMPL_SIG;
      QACTIVE_POST(((BspI2CEvt_t*)me->EvtRef)->AO_sender, evt, (QActive*)me);

      status_ = Q_TRAN(&bsp_i2c_idle);
      break;
    }

    case BSP_I2C_TIMEOUT_SIG: {

      QEvt* const evt = (QEvt* const)(me->EvtRef);

      /* When It's done, go back to idle */
      evt->sig = BSP_I2C_REQ_ERROR_SIG;
      QACTIVE_POST(((BspI2CEvt_t*)me->EvtRef)->AO_sender, evt, (QActive*)me);

      status_ = Q_TRAN(&bsp_i2c_idle);
      break;
    }

    default: {
      status_ = Q_SUPER(&bsp_i2c_busy);
      break;
    }
  }
  return status_;
}

/**
 * @brief The receiving state of the bsp_ext_bus_t state machine.
 *
 * In this state, the bsp_ext_bus_t state machine receives the data
 * contained in the me->rxEvtRef event. When the reception is
 * completed, the state machine transitions back to the bsp_i2c_idle
 * state. If another EXT_BUS_RECEIVE_SIG event is received while the
 * reception is in progress, it is deferred until the reception
 * is completed.
 *
 * @param[in] me Pointer to the bsp_ext_bus_t object.
 * @param[in] e Pointer to the event that triggered the transition
 *              to this state.
 *
 * @return QState value indicating the status of the transition.
 */
static QState
bsp_i2c_receiving(bsp_i2c_t* const me, QEvt const* const e) {
  QState status_;
  switch (e->sig) {
    case Q_ENTRY_SIG: {

      me->reminder = Q_STATE_CAST(&bsp_i2c_receiving);

      /* subAddress = 0x01, data = g_master_txBuff - write to slave.
      start + slaveaddress(w) + subAddress + length of data buffer + data buffer + stop*/
      me->masterXfer.slaveAddress = I2C_MASTER_SLAVE_ADDR_7BIT;
      me->masterXfer.direction = kLPI2C_Read;
      me->masterXfer.subaddress = (uint32_t)((BspI2CEvt_t*)me->EvtRef)->DevAddress;
      me->masterXfer.subaddressSize = 1;
      me->masterXfer.data = ((BspI2CEvt_t*)me->EvtRef)->pui8_data;
      me->masterXfer.dataSize = ((BspI2CEvt_t*)me->EvtRef)->len;
      me->masterXfer.flags = kLPI2C_TransferDefaultFlag;

      /* Send master non-blocking data to slave */
      if (LPI2C_MasterTransferNonBlocking(I2C_MASTER_BASE, &(me->transfer_handle), &(me->masterXfer))
          != kStatus_Success) {
        QEvt* const evt = (QEvt* const)(e);
        evt->sig = BSP_I2C_REQ_ERROR_SIG;
        QACTIVE_POST(((BspI2CEvt_t*)me->EvtRef)->AO_sender, evt, (QActive*)me);
        status_ = Q_TRAN(&bsp_i2c_idle);
      } else {
        status_ = Q_HANDLED();
      }

      status_ = Q_HANDLED();
      break;
    }

    case BSP_I2C_TX_RX_CMPL_SIG: {

      QEvt* const evt = (QEvt* const)(me->EvtRef);

      /* When It's done, go back to idle */
      evt->sig = BSP_I2C_REQ_CMPL_SIG;
      QACTIVE_POST(((BspI2CEvt_t*)me->EvtRef)->AO_sender, evt, me);

      status_ = Q_TRAN(&bsp_i2c_idle);
      break;
    }

    case BSP_I2C_TIMEOUT_SIG: {

      QEvt* const evt = (QEvt* const)(me->EvtRef);

      /* When It's done, go back to idle */
      evt->sig = BSP_I2C_REQ_ERROR_SIG;
      QACTIVE_POST(((BspI2CEvt_t*)me->EvtRef)->AO_sender, evt, (QActive*)me);

      status_ = Q_TRAN(&bsp_i2c_idle);
      break;
    }

    default: {
      status_ = Q_SUPER(&bsp_i2c_busy);
      break;
    }
  }
  return status_;
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

  const port_pin_config_t port3_27_pin34_config = {/* Internal pull-up resistor is enabled */
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
  /* PORT3_27 (pin 34) is configured as LPI2C0_SCL */
  PORT_SetPinConfig(PORT3, 27U, &port3_27_pin34_config);

  const port_pin_config_t port3_28_pin33_config = {/* Internal pull-up resistor is enabled */
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
  PORT_SetPinConfig(PORT3, 28U, &port3_28_pin33_config);
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

/**
 * @brief LPI2C master callback function.
 *
 * This function is called by the LPI2C master peripheral when a transmission or reception
 * is complete. It posts an event to the active object's event queue to notify the
 * application that a transmission or reception has completed.
 *
 * @param base Pointer to the LPI2C master peripheral.
 * @param handle Pointer to the LPI2C master handle.
 * @param status Status of the transmission or reception.
 * @param userData Pointer to the active object which owns this callback function.
 */
static void
lpi2c_master_callback(LPI2C_Type* base, lpi2c_master_handle_t* handle, status_t status, void* userData) {
  bsp_i2c_t* me = (bsp_i2c_t*)userData;

  if (status != kStatus_Success) {
    static const QEvt evt = QEVT_INITIALIZER(BSP_I2C_TX_RX_ERROR_SIG);
    QACTIVE_POST(me->container, &evt, 0U);
  } else {
    static const QEvt evt = QEVT_INITIALIZER(BSP_I2C_TX_RX_CMPL_SIG);
    QACTIVE_POST(me->container, &evt, 0U);
  }
}