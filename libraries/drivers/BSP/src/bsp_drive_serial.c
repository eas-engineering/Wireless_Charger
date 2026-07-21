/******************************************************************************
* Filename              :   bsp_drive_serial.c
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   25 September 2024
*
* Copyright (c) 2024 EAS SPA. All rights reserved.  
*
******************************************************************************/

/** @file bsp_drive_serial.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include "bsp_drive_serial.h"
#include "fsl_clock.h"
#include "fsl_common.h"
#include "fsl_ctimer.h"
#include "fsl_edma.h"
#include "fsl_gpio.h"
#include "fsl_lpuart.h"
#include "fsl_lpuart_edma.h"
#include "fsl_port.h"
#include "global_signals.h"
#include "qpc.h"

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/
#define FIFO_LEN         BSP_DRIVE_SERIAL_BUFF_LEN

#define LPUART           LPUART1
#define LPUART_IRQn      LPUART1_IRQn

#define RS485_RX_PIN     8U
#define RS485_RX_GPIO    GPIO3
#define RS485_RX_PORT    PORT3

#define RS485_TX_PIN     9U
#define RS485_TX_GPIO    GPIO3
#define RS485_TX_PORT    PORT3

#define RS485_DE_PIN     11U
#define RS485_DE_GPIO    GPIO3
#define RS485_DE_PORT    PORT3

#define RX_TIMEOUT_NB    (10U * 4U) /* (1 bit Start + 8 bits Data + 1 bit Stop) x 3.5 times = 40 bits */

#define RS485_DE_SET()   GPIO_PinWrite(RS485_DE_GPIO, RS485_DE_PIN, 1)
#define RS485_DE_CLEAR() GPIO_PinWrite(RS485_DE_GPIO, RS485_DE_PIN, 0)

#define PCR_IBE_ibe1     0x01u /*!<@brief Input Buffer Enable: Enables */

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

/*****************************************************************************
* Module Typedefs
******************************************************************************/

typedef struct {

  /* Super class */
  QHsm super;

  /* Buffer to store data received */
  uint8_t fifo[FIFO_LEN];

  /* Position in the fifo buffer */
  uint32_t head;
  uint32_t tail;

  uint32_t tx_index;

  QEvt const* txEvtRef;

  QActive* container;

} bsp_drive_serial_t;

/*****************************************************************************
* Function Prototypes
******************************************************************************/

static QState drive_serial_initial(bsp_drive_serial_t* const me, void const* const par);
static QState drive_serial_idle(bsp_drive_serial_t* const me, QEvt const* const e);
static QState drive_serial_trasmitting(bsp_drive_serial_t* const me, QEvt const* const e);

static void initialize_serial_interface(void);
static void initialize_pins(void);

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

static bsp_drive_serial_t Serial_inst;

#ifdef Q_SPY
static QSpyId const l_bsp_drive_serial_on_irq = {0U};
static QSpyId const l_bsp_drive_serial_on_tx_dma_irq = {0U};
static QSpyId const l_bsp_drive_serial_on_rx_dma_irq = {0U};
#endif

/*****************************************************************************
* Function Definitions
******************************************************************************/
void
bsp_drive_serial_isr_rx_handler(void) {

  QK_ISR_ENTRY();

  bsp_drive_serial_t* me = &Serial_inst;
  uint8_t data;

  /* If new data request arrived. */
  if ((kLPUART_RxDataRegFullFlag & LPUART_GetStatusFlags(LPUART)) != 0) {
    data = LPUART_ReadByte(LPUART);
    me->fifo[me->head] = data;
    me->head = (me->head + 1U) % FIFO_LEN;
  }

  /* IDLE line detected */
  if ((kLPUART_IdleLineFlag & LPUART_GetStatusFlags(LPUART)) != 0) {
    LPUART_ClearStatusFlags(LPUART, kLPUART_IdleLineFlag);
    static QEvt const evt = QEVT_INITIALIZER(SERIAL_RX_CMPL_SIG);
    QACTIVE_POST(me->container, &evt, me);
  }

  /* IDLE line detected */
  if (((kLPUART_TransmissionCompleteFlag & LPUART_GetStatusFlags(LPUART)) != 0)
      && (kLPUART_TxDataRegEmptyInterruptEnable & LPUART_GetEnabledInterrupts(LPUART)) != 0) {

    LPUART_ClearStatusFlags(LPUART, kLPUART_TransmissionCompleteFlag);

    DriveSerialEvt_t* evt = (DriveSerialEvt_t*)(me->txEvtRef);
    me->tx_index++;

    if (me->tx_index >= evt->len) {
      LPUART_DisableInterrupts(LPUART, kLPUART_TxDataRegEmptyInterruptEnable);
      static QEvt const evt = QEVT_INITIALIZER(SERIAL_TX_CMPL_SIG);
      QACTIVE_POST(me->container, &evt, me);
    } else {
      LPUART_WriteByte(LPUART, evt->pui8_data[me->tx_index]);
    }
  }

  QK_ISR_EXIT();
}

/**
 * @brief Initialize the Drive serial HSM
 *
 * @details This function initializes the Drive serial HSM and sets its initial
 *          state to the idle state.
 */
QHsm*
bsp_drive_serial_init(QActive* const container) {

  bsp_drive_serial_t* me = &Serial_inst;
  initialize_pins();
  initialize_serial_interface();

  QS_FUN_DICTIONARY(&drive_serial_initial);
  QS_FUN_DICTIONARY(&drive_serial_idle);
  QS_FUN_DICTIONARY(&drive_serial_trasmitting);

  QS_SIG_DICTIONARY(SERIAL_SEND_SIG, (void*)0);
  QS_SIG_DICTIONARY(SERIAL_RECEIVE_SIG, (void*)0);
  QS_SIG_DICTIONARY(SERIAL_RX_CMPL_SIG, (void*)0);
  QS_SIG_DICTIONARY(SERIAL_TX_CMPL_SIG, (void*)0);

  me->container = container;

  QHsm_ctor(&(me->super), Q_STATE_CAST(&drive_serial_initial));

  return &(me->super);
}

/**
 * @brief Initial state of the Drive serial HSM
 *
 * @details This state is the topmost initial state of the Drive serial HSM.
 *          It is the source state for the initial transition to the idle state.
 *
 * @param[in] me  Reference to the bsp_drive_serial_t
 * @param[in] par Reference to the parameter of the initial transition
 *
 * @return QState
 */
static QState
drive_serial_initial(bsp_drive_serial_t* const me, void const* const par) {

  QS_OBJ_DICTIONARY(&Serial_inst);

  QS_FUN_DICTIONARY(&drive_serial_initial);
  QS_FUN_DICTIONARY(&drive_serial_idle);
  QS_FUN_DICTIONARY(&drive_serial_trasmitting);

  /* Set DE pin to low */
  RS485_DE_CLEAR();

  (void)par; // unused parameter
  return Q_TRAN(&drive_serial_idle);
}

/**
 * @brief Idle state of the drive serial
 *
 * @details This state is the root state of the Drive serial HSM. The state
 *          machine remains in this state until a command to transmit data
 *          is received.
 *
 * @param[in] me  Reference to the bsp_drive_serial_t
 * @param[in] e   Event that triggered this transition
 */
static QState
drive_serial_idle(bsp_drive_serial_t* const me, QEvt const* const e) {
  QState status_;
  switch (e->sig) {
    case Q_ENTRY_SIG: {
      status_ = Q_HANDLED();
      break;
    }

    case SERIAL_RX_CMPL_SIG: {

      uint32_t pos;
      DriveSerialEvt_t* evt;

      /* Calculate current position in buffer and check for new data available */
      pos = me->head;
      if (pos != me->tail) {  /* Check change in received data */
        if (pos > me->tail) { /* Current position is over previous one */
          /*
           * Processing is done in "linear" mode.
           *
           * Application processing is fast with single data block,
           * length is simply calculated by subtracting pointers
           *
           * [   0   ]
           * [   1   ] <- old_pos |------------------------------------|
           * [   2   ]            |                                    |
           * [   3   ]            | Single block (len = pos - old_pos) |
           * [   4   ]            |                                    |
           * [   5   ]            |------------------------------------|
           * [   6   ] <- pos
           * [   7   ]
           * [ N - 1 ]
           */

          /* Alloco lo spazio per l'evento */
          evt = Q_NEW(DriveSerialEvt_t, SERIAL_RECEIVE_SIG);
          evt->len = pos - me->tail;
          memcpy(evt->pui8_data, &me->fifo[me->tail], evt->len);
          QACTIVE_POST(me->container, (QEvt*)evt, me);
        } else {
          /*
           * Processing is done in "overflow" mode..
           *
           * Application must process data twice,
           * since there are 2 linear memory blocks to handle
           *
           * [   0   ]            |---------------------------------|
           * [   1   ]            | Second block (len = pos)        |
           * [   2   ]            |---------------------------------|
           * [   3   ] <- pos
           * [   4   ] <- old_pos |---------------------------------|
           * [   5   ]            |                                 |
           * [   6   ]            | First block (len = N - old_pos) |
           * [   7   ]            |                                 |
           * [ N - 1 ]            |---------------------------------|
           */
          /* Alloco lo spazio per l'evento */
          evt = Q_NEW(DriveSerialEvt_t, SERIAL_RECEIVE_SIG);
          evt->len = (FIFO_LEN - me->tail) + pos;
          memcpy(evt->pui8_data, &me->fifo[me->tail], FIFO_LEN - me->tail);

          if (pos > 0) {
            memcpy(&evt->pui8_data[FIFO_LEN - me->tail], &me->fifo[0], pos);
          }

          QACTIVE_POST(me->container, (QEvt*)evt, me);
        }
        me->tail = pos; /* Save current position as old for next transfers */
      }

      status_ = Q_HANDLED();
      break;
    }

    case SERIAL_SEND_SIG: {

      Q_NEW_REF(me->txEvtRef, QEvt);
      status_ = Q_TRAN(&drive_serial_trasmitting);
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
 * @brief Drive serial trasmitting state
 *
 * @param[in] me  Reference to the bsp_drive_serial_t
 * @param[in] e   Event that triggered this transition
 *
 * @return QState
 *
 * @details State where the serial trasmission is in progress.
 *          The trasmission is done using the FIFO buffer.
 *          The trasmission is done using the bsp_drive_serial_trasmit() function.
 */
static QState
drive_serial_trasmitting(bsp_drive_serial_t* const me, QEvt const* const e) {
  QState status_;
  switch (e->sig) {
    case Q_ENTRY_SIG: {

      /* Set DE pin to high */
      RS485_DE_SET();

      DriveSerialEvt_t* evt = (DriveSerialEvt_t*)(me->txEvtRef);
      me->tx_index = 0U;
      LPUART_WriteByte(LPUART, evt->pui8_data[0]);
      LPUART_EnableInterrupts(LPUART, kLPUART_TxDataRegEmptyInterruptEnable);

      status_ = Q_HANDLED();
      break;
    }

    case Q_EXIT_SIG: {

      Q_DELETE_REF(me->txEvtRef);

      status_ = Q_HANDLED();
      break;
    }

    case SERIAL_TX_CMPL_SIG: {

      /* Set DE pin to low */
      RS485_DE_CLEAR();

      status_ = Q_TRAN(&drive_serial_idle);
      break;
    }

    default: {
      status_ = Q_SUPER(&drive_serial_idle);
      break;
    }
  }
  return status_;
}

/**
 * @brief Initialize the USART interface
 *
 * @details This function initializes the USART interface in asynchronous
 *          mode. The USART is configured to work at 9600 bps with 8 bits of
 *          data, no parity and one stop bit. The FIFO is enabled and set to
 *          use 7/8 of the available memory. The DMA is enabled for both RX
 *          and TX. The idle interrupt is enabled.
 */
static void
initialize_serial_interface(void) {
  lpuart_config_t config;

  // 1. Abilita clock periferica (se necessario)
  //CLOCK_EnableClock(kCLOCK_GateLPUART1);
  CLOCK_SetClockDiv(kCLOCK_DivLPUART1, 1u);
  // 2. Seleziona sorgente clock
  CLOCK_AttachClk(kFRO12M_to_LPUART1);

  // 3. RESET periferica → QUI
  RESET_PeripheralReset(kLPUART1_RST_SHIFT_RSTn);

  /* 2. Configurazione base */
  LPUART_GetDefaultConfig(&config);
  config.baudRate_Bps = 115200U;
  config.dataBitsCount = kLPUART_EightDataBits;
  config.parityMode = kLPUART_ParityDisabled;
  config.stopBitCount = kLPUART_OneStopBit;
  config.enableTx = true;
  config.enableRx = true;
  config.txFifoWatermark = 0U;
  config.rxFifoWatermark = 0U;

  /* 3. Init periferica */
  LPUART_Init(LPUART, &config, 12000000U);

  /* 5. RX timeout / idle detection */
  LPUART_EnableInterrupts(LPUART, kLPUART_RxDataRegFullInterruptEnable | kLPUART_IdleLineInterruptEnable);
  EnableIRQ(LPUART_IRQn);
  NVIC_SetPriority(LPUART_IRQn, 5U);
}

/**
 * @brief Initialize the pins for USART
 *
 * @details This function initializes the pins associated with the USART
 *          interface. The pins are set to alternate function 7 to use the
 *          USART1 peripheral.
 */
static void
initialize_pins(void) {
  /* Abilita clock delle porte GPIO coinvolte */
  CLOCK_EnableClock(kCLOCK_GatePORT3);
  CLOCK_EnableClock(kCLOCK_GateGPIO3);

  /* Rilascia reset delle porte */
  RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);
  RESET_ReleasePeripheralReset(kGPIO3_RST_SHIFT_RSTn);

  /* LPUART1 peripheral is released from reset */
  //RESET_ReleasePeripheralReset(kLPUART1_RST_SHIFT_RSTn);

  /************************************************************
     *  CONFIGURAZIONE PIN DI USCITA
     ************************************************************/
  const gpio_pin_config_t out_cfg_de = {.pinDirection = kGPIO_DigitalOutput, .outputLogic = 0u};

  /* --------------------- RS485_RX_PIN (P3_8) --------------------- */
  const port_pin_config_t port3_8_cfg = {kPORT_PullUp,
                                         kPORT_LowPullResistor,
                                         kPORT_FastSlewRate,
                                         kPORT_PassiveFilterDisable,
                                         kPORT_OpenDrainDisable,
                                         kPORT_LowDriveStrength,
                                         kPORT_NormalDriveStrength,
                                         kPORT_MuxAlt3,
                                         kPORT_InputBufferEnable,
                                         kPORT_InputNormal,
                                         kPORT_UnlockRegister};
  PORT_SetPinConfig(PORT3, RS485_RX_PIN, &port3_8_cfg);

  /* --------------------- RS485_TX_PIN (P3_9) --------------------- */
  const port_pin_config_t port3_9_cfg = {kPORT_PullUp,
                                         kPORT_LowPullResistor,
                                         kPORT_FastSlewRate,
                                         kPORT_PassiveFilterDisable,
                                         kPORT_OpenDrainDisable,
                                         kPORT_LowDriveStrength,
                                         kPORT_NormalDriveStrength,
                                         kPORT_MuxAlt3,
                                         kPORT_InputBufferEnable,
                                         kPORT_InputNormal,
                                         kPORT_UnlockRegister};
  PORT_SetPinConfig(PORT3, RS485_TX_PIN, &port3_9_cfg);

  // /* PORT3_8 (pin 21) is configured as LPUART1_RXD */
  // PORT_SetPinMux(PORT3, 8U, kPORT_MuxAlt3);

  // PORT3->PCR[8] = ((PORT3->PCR[8] &
  //                   /* Mask bits to zero which are setting */
  //                   (~(PORT_PCR_IBE_MASK)))

  //                  /* Input Buffer Enable: Enables. */
  //                  | PORT_PCR_IBE(PCR_IBE_ibe1));

  // /* PORT3_9 (pin 20) is configured as LPUART1_TXD */
  // PORT_SetPinMux(PORT3, 9U, kPORT_MuxAlt3);

  // PORT3->PCR[9] = ((PORT3->PCR[9] &
  //                   /* Mask bits to zero which are setting */
  //                   (~(PORT_PCR_IBE_MASK)))

  //                  /* Input Buffer Enable: Enables. */
  //                  | PORT_PCR_IBE(PCR_IBE_ibe1));

  /* --------------------- RS485_DE_PIN (P3_11) --------------------- */
  const port_pin_config_t port3_11_cfg = {
    kPORT_PullDown,           kPORT_LowPullResistor,  kPORT_FastSlewRate,        kPORT_PassiveFilterDisable,
    kPORT_OpenDrainDisable,   kPORT_LowDriveStrength, kPORT_NormalDriveStrength, kPORT_MuxAsGpio,
    kPORT_InputBufferDisable, kPORT_InputNormal,      kPORT_UnlockRegister};
  PORT_SetPinConfig(PORT3, RS485_DE_PIN, &port3_11_cfg);
  GPIO_PinInit(GPIO3, RS485_DE_PIN, &out_cfg_de);
}
