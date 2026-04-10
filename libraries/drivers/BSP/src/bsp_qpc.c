/******************************************************************************
* Filename              :   bsp_qpc.c
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   02 August 2024
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

/** @file bsp_qpc.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include "bsp_qpc.h"
#include "bsp_qpc_spy.h"
#include "bsp_tick.h"
#include "project_settings.h"
#include "qpc.h"

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/

#define BSP_TICKS_PER_SEC 1000U

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

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!! CAUTION !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* Assign a priority to EVERY ISR explicitly by calling NVIC_SetPriority().
* DO NOT LEAVE THE ISR PRIORITIES AT THE DEFAULT VALUE! (see NOTE0)
*/
enum KernelUnawareISRs {
  TIM_HALL_SENSOR_PRIO,
  MAX_KERNEL_UNAWARE_CMSIS_PRI /* keep always last */
};

/* "kernel-unaware" interrupts can't overlap "kernel-aware" interrupts */
Q_ASSERT_COMPILE(MAX_KERNEL_UNAWARE_CMSIS_PRI <= QF_AWARE_ISR_CMSIS_PRI);

enum KernelAwareISRs {
  BSP_I2C_BUS_PRIO = QF_AWARE_ISR_CMSIS_PRI,
  MAX_KERNEL_AWARE_CMSIS_PRI /* keep always last */
};

/* "kernel-aware" interrupts should not overlap the PendSV priority */
Q_ASSERT_COMPILE(MAX_KERNEL_AWARE_CMSIS_PRI <= (0xFF >> (8 - __NVIC_PRIO_BITS)));

#ifdef Q_SPY
// QSpy source IDs
QSpyId const l_SysTick_Handler = {0U};
#endif

/*****************************************************************************
* Function Definitions
******************************************************************************/

/**
 * @brief 
 * 
 */
void
QF_onStartup(void) {
  // set up the SysTick timer to fire at BSP_TICKS_PER_SEC rate
  SysTick_Config(SystemCoreClock / BSP_TICKS_PER_SEC);

  // assign all priority bits for preemption-prio. and none to sub-prio.
  NVIC_SetPriorityGrouping(0U);

  // set priorities of ALL ISRs used in the system, see NOTE1
  NVIC_SetPriority(LPI2C0_IRQn, BSP_I2C_BUS_PRIO);
}

/**
 * @brief 
 * 
 */
void
QF_onCleanup(void) {}

/**
 * @brief 
 * 
 */
void
QK_onIdle(void) {
#ifdef Q_SPY
  bsp_qpc_spy_process();
#endif
}

#ifndef Q_UTEST
/**
 * @brief 
 * 
 * @param module 
 * @param id 
 * @return Q_NORETURN 
 */
Q_NORETURN
Q_onError(char const* module, int_t const id) {
  // NOTE: this implementation of the assertion handler is intended only
  // for debugging and MUST be changed for deployment of the application
  // (assuming that you ship your production code with assertions enabled).
  Q_UNUSED_PAR(module);
  Q_UNUSED_PAR(id);
  QS_ASSERTION(module, id, (uint32_t)10000U);

#if DEBUG
  // for debugging, hang on in an endless loop...
  for (;;) {}
#endif

  NVIC_SystemReset();
}
#endif

//============================================================================
// NOTE1:
// The QF_AWARE_ISR_CMSIS_PRI constant from the QF port specifies the highest
// ISR priority that is disabled by the QF framework. The value is suitable
// for the NVIC_SetPriority() CMSIS function.
//
// Only ISRs prioritized at or below the QF_AWARE_ISR_CMSIS_PRI level (i.e.,
// with the numerical values of priorities equal or higher than
// QF_AWARE_ISR_CMSIS_PRI) are allowed to call the QK_ISR_ENTRY/
// QK_ISR_ENTRY macros or any other QF/QK services. These ISRs are
// "QF-aware".
//
// Conversely, any ISRs prioritized above the QF_AWARE_ISR_CMSIS_PRI priority
// level (i.e., with the numerical values of priorities less than
// QF_AWARE_ISR_CMSIS_PRI) are never disabled and are not aware of the kernel.
// Such "QF-unaware" ISRs cannot call ANY QF/QK services. In particular they
// can NOT call the macros QK_ISR_ENTRY/QK_ISR_ENTRY. The only mechanism
// by which a "QF-unaware" ISR can communicate with the QF framework is by
// triggering a "QF-aware" ISR, which can post/publish events.
//
// NOTE2:
// The User LED is used to visualize the idle loop activity. The brightness
// of the LED is proportional to the frequency of the idle loop.
// Please note that the LED is toggled with interrupts locked, so no interrupt
// execution time contributes to the brightness of the User LED.
//

/**
  * @brief This function handles System tick timer.
  */
void
SysTick_Handler(void) {
  QK_ISR_ENTRY(); // inform QK about entering an ISR

  QTIMEEVT_TICK_X(0U, &l_SysTick_Handler); // time events at rate 0

  bsp_tick_inc();

#ifdef Q_SPY
  bsp_qpc_spy_tick();
#endif

  QK_ISR_EXIT(); // inform QK about exiting an ISR
}
