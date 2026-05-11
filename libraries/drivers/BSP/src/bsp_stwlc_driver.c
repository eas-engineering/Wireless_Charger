/******************************************************************************
 * Filename              : bsp_stwlc_driver.c
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

/** @file bsp_stwlc_driver.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include "bsp_stwlc_driver.h"
#include "bsp_i2c.h"

// Q_DEFINE_THIS_FILE // define the name of this file for assertions

//   /*****************************************************************************
// * Module Preprocessor Constants
// ******************************************************************************/

//   /*****************************************************************************
// * Module Preprocessor Macros
// ******************************************************************************/

//   /*****************************************************************************
// * Module Typedefs
// ******************************************************************************/

//   typedef struct {
//   // protected:
//   QActive super; // inherit QActive

//   // private:
//   QTimeEvt timeEvt; // private time event generator

//   QHsm* I2cDriver; /* Pointer to the I2C driver HSM */

// } BspStwlcDriver_t;

// /*****************************************************************************
// * Function Prototypes
// ******************************************************************************/

// static QState bsp_stwlc_initial_state(BspStwlcDriver_t* const me, void const* const par);
// static QState bsp_stwlc_active_state(BspStwlcDriver_t* const me, QEvt const* const e);

// /*****************************************************************************
// * Module Variable Definitions
// ******************************************************************************/

// static BspStwlcDriver_t BspStwlcDriver;
// QActive* const AO_BspStwlcDriver = &BspStwlcDriver.super;

// /*****************************************************************************
// * Function Definitions
// ******************************************************************************/

// /**
//  * @brief Initialize the BSP STWLC driver state machine.
//  *
//  * This function initializes the BSP STWLC driver state machine.
//  * It creates the state machine and sets up the event queue and the
//  * time event generator. It also sets up the QS dictionary for
//  * introspection and sets up the QF dictionary for function
//  * tracing.
//  *
//  * @note This function must be called before the BSP STWLC driver
//  * state machine is started.
//  */
// void
// bsp_stwlc_driver_init(void) {

//   static QEvt const* DriveMgrQueueSto[10];

//   BspStwlcDriver_t* const me = &BspStwlcDriver;
//   QActive_ctor(&me->super, Q_STATE_CAST(&bsp_stwlc_initial_state));

//   QTimeEvt_ctorX(&me->timeEvt, &me->super, TIMEOUT_SIG, 0U);

//   QS_OBJ_DICTIONARY(&BspStwlcDriver);
//   QS_OBJ_DICTIONARY(&BspStwlcDriver.timeEvt);

//   QS_FUN_DICTIONARY(&bsp_stwlc_initial_state);
//   QS_FUN_DICTIONARY(&bsp_stwlc_active_state);

//   QS_SIG_DICTIONARY(SERIAL_SEND_SIG, (void*)0);

//   QACTIVE_START(AO_BspStwlcDriver,
//                 10U,                     // QP prio. of the AO
//                 DriveMgrQueueSto,        // event queue storage
//                 Q_DIM(DriveMgrQueueSto), // queue length [events]
//                 (void*)0, 0U,            // no stack storage
//                 (void*)0);               // no initialization param
// }

// /**
//  * @brief Initial state handler for the bsp_stwlc_driver_t state machine.
//  *
//  * This function is the initial state handler for the bsp_stwlc_driver_t state machine.
//  * It initializes the I2C driver and transitions to the bsp_stwlc_active_state.
//  *
//  * @param me Pointer to the bsp_stwlc_driver_t object.
//  * @param par Unused parameter.
//  *
//  * @return Q_TRAN to transition to the bsp_stwlc_active_state state.
//  */
// static QState
// bsp_stwlc_initial_state(BspStwlcDriver_t* const me, void const* const par) {
//   Q_UNUSED_PAR(par);
//   me->I2cDriver = bsp_i2c_init(AO_BspStwlcDriver);
//   QASM_INIT(me->I2cDriver, (void*)0, me->super.prio);
//   return Q_TRAN(&bsp_stwlc_active_state);
// }

// /**
//  * @brief Active state handler for the bsp_stwlc_driver_t state machine.
//  *
//  * This function is the active state handler for the bsp_stwlc_driver_t state machine.
//  * It processes the Q_ENTRY_SIG event and dispatches the I2C driver for the
//  * BSP_I2C_TX_RX_CMPL_SIG and BSP_I2C_TX_RX_ERROR_SIG events.
//  *
//  * @param me Pointer to the bsp_stwlc_driver_t object.
//  * @param e Pointer to the event that triggered the transition to this state.
//  *
//  * @return Q_TRAN to transition to the bsp_stwlc_active_state state.
//  */
// static QState
// bsp_stwlc_active_state(BspStwlcDriver_t* const me, QEvt const* const e) {
//   QState status;
//   switch (e->sig) {

//     case Q_ENTRY_SIG: {

//       QTimeEvt_armX(&me->timeEvt, 1000U, 0U);

//       status = Q_HANDLED();
//       break;
//     }

//     case BSP_I2C_TX_RX_CMPL_SIG:
//     case BSP_I2C_TX_RX_ERROR_SIG:
//     case BSP_I2C_TIMEOUT_SIG: {

//       QASM_DISPATCH(me->I2cDriver, e, 0U);

//       status = Q_HANDLED();
//       break;
//     }

//     case TIMEOUT_SIG: {

//       BspI2CEvt_t* const evt = Q_NEW(BspI2CEvt_t, BSP_I2C_SEND_SIG);
//       evt->DevAddress = 0x2AU;
//       evt->MemAddress = 0x00U;
//       evt->len = 10U;
//       evt->AO_sender = AO_BspStwlcDriver;
//       QASM_DISPATCH(me->I2cDriver, (QEvt*)evt, 0U);

//       status = Q_HANDLED();
//       break;
//     }

//     case BSP_I2C_REQ_CMPL_SIG: {

//       QTimeEvt_armX(&me->timeEvt, 1000U, 0U);

//       status = Q_HANDLED();
//       break;
//     }

//     case BSP_I2C_REQ_ERROR_SIG: {

//       QTimeEvt_armX(&me->timeEvt, 1000U, 0U);

//       status = Q_HANDLED();
//       break;
//     }

//     default: {
//       status = Q_SUPER(&QHsm_top);
//       break;
//     }
//   }
//   return status;
// }