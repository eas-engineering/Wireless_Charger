/******************************************************************************
* Filename              :   modbus_server_manager.c
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   21 March 2025
*
* Copyright (c) 2024 EAS SPA. All rights reserved.  
*
******************************************************************************/

/** @file modbus_server_manager.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include <stddef.h>
#include "bsp_drive_serial.h"
#include "global_signals.h"
#include "mb.h"
#include "mbutils.h"
#include "modbus_server_manager.h"

Q_DEFINE_THIS_FILE

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/

#define FW_NAME_MAX_LEN            (IR_FIRMWARE_REV_BASE_ADDR - IR_FIRMWARE_NAME_BASE_ADDR)
#define CENTER_NAME_MAX_LEN        (IR_EVENT_MANT_TYPE_ADDR - IR_EVENT_MANT_CENTER_NAME_ADDR)
#define NOTE_MAX_LEN               (IR_DUMMY_ADDR - IR_EVENT_MANT_NOTE_ADDR)

#define HR_BASE_ADDR               1000U
#define HR_VBAT_ADDR               (HR_BASE_ADDR + 0U)
#define HR_IBAT_ADDR               (HR_BASE_ADDR + 1U)
#define HR_TBAT_ADDR               (HR_BASE_ADDR + 2U)
#define HR_SOC_ADDR                (HR_BASE_ADDR + 3U)
#define HR_END_OF_CHARGE_TIME_ADDR (HR_BASE_ADDR + 4U)
#define HR_NUMBER_OF_CHARGES_ADDR  (HR_BASE_ADDR + 5U)
#define HR_DUMMY_ADDR              (HR_BASE_ADDR + 6U)

#define HR_REG_OFF(x)              ((x) - HR_BASE_ADDR)
#define HR_GROUP_LEN               (HR_DUMMY_ADDR - HR_BASE_ADDR + 1U)

#define PASSWORD_TIMEOUT_MS        60000U

/* Modbus register limits */
#define MAX_RW_REGS_PER_REQUEST    17U

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

/*****************************************************************************
* Module Typedefs
******************************************************************************/

/**
 * @brief Structure used to manage the modbus server manager
 * 
 */
typedef struct {
  // protected:
  QActive super; // inherit QActive

  // private:
  QTimeEvt timeEvt;          // private time event generator
  QTimeEvt timeEvtUnbloking; // private time event generator

  MbChannel_t RTUSerialChannel;
  MbIFaceConfigUART_t IFaceConfigUART;

  MbChannel_t RTUUSBChannel;
  MbIFaceConfigUART_t IFaceConfigUSB;

  QHsm* serialDriver; /* Pointer to the serial driver HSM */
  QHsm* usb_manager;  /* Pointer to the USB manager HSM */

  uint16_t holding_register_buff[HR_GROUP_LEN];

} ModbusServerManager_t;

/**
 * @brief Modbus server manager event structure
 * 
 */
typedef struct {
  QEvt super;
  uint16_t address;
  uint16_t num_of_regs;
  uint8_t buffer[MAX_RW_REGS_PER_REQUEST * 2]; // Buffer to hold the data for read/write operations
} ModbusManagerEvt_t;

/*****************************************************************************
* Function Prototypes
******************************************************************************/

static QState mb_server_initial_state(ModbusServerManager_t* const me, void const* const par);
static QState mb_server_active_state(ModbusServerManager_t* const me, QEvt const* const e);

static mb_error_code_t mb_reg_input(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNRegs, void* user_data);
static mb_error_code_t mb_reg_holding(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode,
                                      void* user_data);
#if COIL_NCOILS > 0
static mb_error_code_t mb_reg_coils(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode,
                                    void* user_data);
#endif
static mb_error_code_t mb_reg_discrete(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNDiscrete, void* user_data);

static void modbus_rtu_send_data(MbIFaceConfigUART_t* pConfig, const UCHAR* pData, ULONG NumBytes);
static void modbus_rtu_usart_init(MbIFaceConfigUART_t* pConfig);
static void modbus_rtu_usart_deinit(MbIFaceConfigUART_t* pConfig);

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

static const MbSlaveAPI_t MbSlaveAPI = {
  mb_reg_input,
  mb_reg_holding,
#if COIL_NCOILS > 0
  mb_reg_coils,
#else
  NULL,
#endif
  mb_reg_discrete,
};

static const MbIFaceUSARTAPI_t IFaceUARTAPI = {
  NULL,                    // pfSendByte
  modbus_rtu_usart_init,   // pfInit
  modbus_rtu_usart_deinit, // pfDeInit
  modbus_rtu_send_data,    // pfSend
  NULL,                    // pfRecv
  NULL,                    // pfConnect
  NULL,                    // pfDisconnect
};

static ModbusServerManager_t ModbusServerManager;
QActive* const AO_ModbusServerManager = &ModbusServerManager.super;

/*****************************************************************************
* Function Definitions
******************************************************************************/

/**
 * @brief Initializes the Modbus server manager.
 *
 * This function initializes the Modbus server manager active object and starts
 * it executing. The manager is responsible for managing the Modbus server
 * and the serial interface it communicates over.
 */
void
modbus_server_manager_init(void) {
  // instantiate and start AOs/threads...

  static QEvt const* ModbusMgrQueueSto[10];

  ModbusServerManager_t* const me = &ModbusServerManager;
  QActive_ctor(&me->super, Q_STATE_CAST(&mb_server_initial_state));
  QTimeEvt_ctorX(&me->timeEvt, &me->super, TIMEOUT_SIG, 0U);
  QTimeEvt_ctorX(&me->timeEvtUnbloking, &me->super, MODBUS_UNBLOCKED_SIG, 0U);

  QS_OBJ_DICTIONARY(&ModbusServerManager);
  QS_OBJ_DICTIONARY(&ModbusServerManager.timeEvt);

  QS_FUN_DICTIONARY(&mb_server_initial_state);
  QS_FUN_DICTIONARY(&mb_server_active_state);

  me->serialDriver = bsp_drive_serial_init((QActive* const)me);

  QACTIVE_START(AO_ModbusServerManager,
                15U,                      // QP prio. of the AO
                ModbusMgrQueueSto,        // event queue storage
                Q_DIM(ModbusMgrQueueSto), // queue length [events]
                (void*)0, 0U,             // no stack storage
                (void*)0);                // no initialization param
}

/**
 * @brief Initializes the initial state of the Modbus server manager.
 *
 * This function sets up the initial state for the Modbus server manager
 * active object. It initializes the Modbus slave, configures the timer
 * frequency, and adds an RTU channel with specified parameters.
 *
 * @param me Pointer to the ModbusServerManager active object.
 * @param par Unused parameter.
 *
 * @return QState Transition to the active state of the Modbus server.
 */

static QState
mb_server_initial_state(ModbusServerManager_t* const me, void const* const par) {
  Q_UNUSED_PAR(par);

  mb_slave_init();

  mb_config_timer_frequency(100);

  mb_slave_add_rtu_channel(&(me->RTUSerialChannel), &(me->IFaceConfigUART), &MbSlaveAPI, &IFaceUARTAPI, 1, 1, 38400, 8,
                           1, MB_PAR_NONE, (void*)me);

  QASM_INIT(me->serialDriver, (void*)0, me->super.prio);

  return Q_TRAN(&mb_server_active_state);
}

/**
 * @brief Active state of the Modbus server manager
 *
 * In this state, the Modbus server manager is running. The timer
 * event is used to call the Modbus timer tick periodically. The
 * MODBUS_NET_EVENT_SIG event is used to process the Modbus protocol.
 *
 * @param me Pointer to the ModbusServerManager active object.
 * @param e Pointer to the event that triggered the transition to this state.
 *
 * @return QState Transition to the next state of the Modbus server manager.
 */
static QState
mb_server_active_state(ModbusServerManager_t* const me, QEvt const* const e) {
  QState status;
  switch (e->sig) {

    case Q_ENTRY_SIG: {
      //QTimeEvt_rearm(&me->timeEvt, 10);
      status = Q_HANDLED();
      break;
    }

    case MODBUS_BATTERY_INFO_UPDATE_SIG: {

      me->holding_register_buff[HR_REG_OFF(HR_VBAT_ADDR)] = (uint16_t)(Q_EVT_CAST(ModBusInfoEvt)->vbat);
      me->holding_register_buff[HR_REG_OFF(HR_IBAT_ADDR)] = (uint16_t)(Q_EVT_CAST(ModBusInfoEvt)->ibat);
      me->holding_register_buff[HR_REG_OFF(HR_TBAT_ADDR)] = (uint16_t)(Q_EVT_CAST(ModBusInfoEvt)->tbat);
      me->holding_register_buff[HR_REG_OFF(HR_SOC_ADDR)] = (uint16_t)(Q_EVT_CAST(ModBusInfoEvt)->soc);
      me->holding_register_buff[HR_REG_OFF(HR_END_OF_CHARGE_TIME_ADDR)] =
        (uint16_t)(Q_EVT_CAST(ModBusInfoEvt)->end_of_charge_time);
      me->holding_register_buff[HR_REG_OFF(HR_NUMBER_OF_CHARGES_ADDR)] =
        (uint16_t)(Q_EVT_CAST(ModBusInfoEvt)->number_of_charges);

      status = Q_HANDLED();
      break;
    }

    case SERIAL_RX_CMPL_SIG:
    case SERIAL_TX_CMPL_SIG: {

      QASM_DISPATCH(me->serialDriver, e, 0U);

      status = Q_HANDLED();
      break;
    }

    case SERIAL_RECEIVE_SIG: {

      DriveSerialEvt_t const* const evt = Q_EVT_CAST(DriveSerialEvt_t);
      mb_slave_poll_channel(&(me->RTUSerialChannel), evt->pui8_data, evt->len);

      status = Q_HANDLED();
      break;
    }

    case MODBUS_NET_EVENT_SIG: {

      mb_slave_process();

      status = Q_HANDLED();
      break;
    }

    default: {
      status = Q_SUPER(&QHsm_top);
      break;
    }
  }
  return status;
}

/**
 * @brief Handle Modbus input registers.
 *
 * @param pucRegBuffer Pointer to the buffer where the register values will be stored.
 * @param usAddress Starting address of the registers.
 * @param usNRegs Number of registers to be processed.
 * @param user_data User-defined data structure, typically pointing to Modbus server manager context.
 *
 * @return mb_error_code_t Error code indicating the status of the operation.
 */
static mb_error_code_t
mb_reg_input(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNRegs, void* user_data) {
  return MB_ENOREG;
}

/**
 * @brief Handle Modbus holding registers.
 *
 * @param pucRegBuffer Pointer to the buffer where the register values will be stored.
 * @param usAddress Starting address of the registers.
 * @param usNRegs Number of registers to be processed.
 * @param eMode Modbus register mode (read or write).
 * @param user_data User-defined data structure, typically pointing to Modbus server manager context.
 *
 * @return mb_error_code_t Error code indicating the status of the operation.
 */
static mb_error_code_t
mb_reg_holding(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode, void* user_data) {

  ModbusServerManager_t* const me = (ModbusServerManager_t* const)user_data;
  mb_error_code_t eStatus = MB_ENOERR;
  int iRegIndex;

  if ((usAddress >= HR_BASE_ADDR) && (usAddress + usNRegs <= (HR_BASE_ADDR + HR_GROUP_LEN))) {
    iRegIndex = (int)(usAddress - HR_BASE_ADDR);
    switch (eMode) {
        /* Pass current register values to the protocol stack. */
      case MB_REG_READ: {
        while (usNRegs > 0) {
          *pucRegBuffer++ = (unsigned char)(me->holding_register_buff[iRegIndex] >> 8);
          *pucRegBuffer++ = (unsigned char)(me->holding_register_buff[iRegIndex] & 0xFF);
          iRegIndex++;
          usNRegs--;
        }
        break;
      }

        /* Update current register values with new values from the
           * protocol stack. */
      case MB_REG_WRITE: {

        ModbusManagerEvt_t* evt = Q_NEW(ModbusManagerEvt_t, MODBUS_WRITE_VALUE_SIG);
        evt->address = usAddress;
        evt->num_of_regs = usNRegs;
        for (uint32_t i = 0; i < usNRegs * 2; i++) {
          evt->buffer[i] = pucRegBuffer[i];
        }
        QACTIVE_POST(AO_ModbusServerManager, (QEvt*)evt, me);

        break;
      }

      default: {
        /* Do nothing, not allowed */
        break;
      }
    }
  } else {
    eStatus = MB_ENOREG;
  }
  return eStatus;
}

#if COIL_NCOILS > 0
/**
 * @brief Handle Modbus coils.
 * 
 * @param pucRegBuffer Pointer to the buffer where the coil values will be stored.
 * @param usAddress Starting address of the coils.
 * @param usNCoils Number of coils to be processed.
 * @param eMode Modbus register mode (read or write).
 * @param user_data User-defined data structure, typically pointing to Modbus server manager context.
 * 
 * @return mb_error_code_t Error code indicating the status of the operation.
 */
static mb_error_code_t
mb_reg_coils(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode, void* user_data) {
  mb_error_code_t eStatus = MB_ENOERR;
  USHORT iRegIndex, iRegBitIndex, iNReg;
  UCHAR* pucCoilBuf;
  USHORT usCoilStart;
  iNReg = usNCoils / 8 + 1;
  ModbusServerManager_t* const me = (ModbusServerManager_t* const)user_data;

  pucCoilBuf = me->ucMCoilBuf;
  usCoilStart = COIL_START;

  usAddress--;
  if ((usAddress >= COIL_START) && (usAddress + usNCoils <= COIL_START + COIL_NCOILS)) {
    iRegIndex = (int)(usAddress - usCoilStart) / 8;
    iRegBitIndex = (int)(usAddress - usCoilStart) % 8;
    switch (eMode) {
      /* Pass current coil values to the protocol stack. */
      case MB_REG_READ:
        while (iNReg > 0) {
          *pucRegBuffer++ = mb_util_get_bits(&pucCoilBuf[iRegIndex++], iRegBitIndex, 8);
          iNReg--;
        }
        pucRegBuffer--;
        usNCoils = usNCoils % 8;
        *pucRegBuffer = *pucRegBuffer << (8 - usNCoils);
        *pucRegBuffer = *pucRegBuffer >> (8 - usNCoils);
        break;

      /* Update current coil values with new values from the
				* protocol stack. */
      case MB_REG_WRITE:
        while (iNReg > 1) {
          mb_util_set_bits(&pucCoilBuf[iRegIndex++], iRegBitIndex, 8, *pucRegBuffer++);
          iNReg--;
        }
        usNCoils = usNCoils % 8;
        if (usNCoils != 0) {
          mb_util_set_bits(&pucCoilBuf[iRegIndex++], iRegBitIndex, usNCoils, *pucRegBuffer++);
        }
        break;
    }
  } else {
    eStatus = MB_ENOREG;
  }
  return eStatus;
}
#endif

/**
 * @brief Handle Modbus discrete inputs.
 * 
 * @param pucRegBuffer Pointer to the buffer where the discrete input values will be stored.
 * @param usAddress Starting address of the discrete inputs.
 * @param usNDiscrete Number of discrete inputs to be processed.
 * @param user_data User-defined data structure, typically pointing to Modbus server manager context.
 * 
 * @return mb_error_code_t Error code indicating the status of the operation.
 */
static mb_error_code_t
mb_reg_discrete(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNDiscrete, void* user_data) {
  return MB_ENOREG;
}

/**
 * @brief Send a byte via USART (Modbus RTU mode)
 *
 * @param[in] pConfig   Pointer to the interface configuration structure
 * @param[in] Data      Data byte to be sent
 *
 * @details This function sends a byte via USART. It is used in Modbus RTU mode.
 */
static void
modbus_rtu_send_data(MbIFaceConfigUART_t* pConfig, const UCHAR* pData, ULONG NumBytes) {

  ModbusServerManager_t* const me = &ModbusServerManager;

  if (1 == pConfig->Port) {
    /* Send a request to the drive manager to obtain the drive information */
    DriveSerialEvt_t* evt = Q_NEW(DriveSerialEvt_t, SERIAL_SEND_SIG);
    for (size_t i = 0; i < NumBytes; i++) {
      evt->pui8_data[i] = pData[i];
    }
    evt->len = NumBytes;
    QASM_DISPATCH(me->serialDriver, (QEvt*)evt, 0U);
  }
}

/**
 * @brief Initialize USART for Modbus RTU mode
 *
 * @param[in] pConfig   Pointer to the interface configuration structure
 *
 * @details This function initializes USART for Modbus RTU mode. It sets up the
 *          callbacks for USART RXNE and TC flags, and then initializes the
 *          USART.
 */
static void
modbus_rtu_usart_init(MbIFaceConfigUART_t* pConfig) {
  /* Nothing to do */
}

/**
 * @brief De-initialize USART for Modbus RTU mode
 *
 * @param[in] pConfig   Pointer to the interface configuration structure
 *
 * @details This function de-initializes USART for Modbus RTU mode.
 */
static void
modbus_rtu_usart_deinit(MbIFaceConfigUART_t* pConfig) {
  /* Nothing to do */
}