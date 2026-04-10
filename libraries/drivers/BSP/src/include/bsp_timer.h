/*****************************************************************************
 * Filename              :   bsp_timer.h
 * Author                :   Giulio Dalla Vecchia
 * Origin Date           :   01 apr 2021
 *
 * THIS SOFTWARE IS PROVIDED BY EAS ELETTRONICA COMPANY "AS IS" AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL EAS ELETTRONICA OR ITS CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/** @file bsp_timer.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef BSP_TIMER_H_
#define BSP_TIMER_H_

/*****************************************************************************
 * Includes
 ******************************************************************************/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup        TF Template file manager
 * \brief
 * \{
 */

/*****************************************************************************
 * Module Preprocessor Constants
 ******************************************************************************/

/*****************************************************************************
 * Module Preprocessor Macros
 ******************************************************************************/

/*****************************************************************************
 * Module Typedefs
 ******************************************************************************/

struct bsp_timer;

typedef void (*bsp_timer_callback_fn_t)(struct bsp_timer const* const _this);

/**
 * @brief  BSP Timer configuration typedef
 */
typedef struct bsp_timer {
  volatile uint32_t ui32_timeout;
  volatile uint32_t ui32_interval;
  bsp_timer_callback_fn_t callback_fn;
} bsp_timer_t;

/*****************************************************************************
 * Module Variable Definitions
 ******************************************************************************/

/*****************************************************************************
 * Function Prototypes
 ******************************************************************************/

void bsp_timer_add(bsp_timer_t* const _this, bsp_timer_callback_fn_t fn);

void bsp_timer_arm(bsp_timer_t* const _this, uint32_t ui32_timeout, uint32_t ui32_interval);

void bsp_timer_disarm(bsp_timer_t* const _this);

/* static (i.e., class-wide) operation */
void bsp_timer_tick(void);

/**
 * \}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*BSP_TIMER_H_*/

/*** End of File *************************************************************/
