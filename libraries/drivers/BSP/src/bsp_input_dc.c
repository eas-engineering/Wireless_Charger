/******************************************************************************
 * Filename              :   bsp_input_dc.c
 * Author                :   Giulio Dalla Vecchia
 * Origin Date           :   18 dic 2020
 *
 * THIS SOFTWARE IS PROVIDED BY EAS ELETTRONICA "AS IS" AND ANY EXPRESSED
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL EAS ELETTRONICA OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/** @file bsp_input_dc.c
 *  @brief This is the source file for doxygen comments function
 */

/******************************************************************************
 * Includes
 *******************************************************************************/
#include <stddef.h>
#include <stdint.h>
#include "bsp_input_dc.h"
#include "fsl_common.h"
#include "fsl_ctimer.h"
#include "fsl_gpio.h"
#include "fsl_inputmux.h"
#include "fsl_lpadc.h"
#include "fsl_port.h"
#include "project_settings.h"
#include "bsp_tick.h"

/******************************************************************************
 * Module Preprocessor Constants
 *******************************************************************************/

/**
 *  DC input line states
 */
#define BSP_DC_INPUT_LINE_STATE_START  0
#define BSP_DC_INPUT_IN_STATE_DEBOUNCE 1

/******************************************************************************
 * Module Preprocessor Macros
 *******************************************************************************/

/******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static bsp_input_error_t bsp_input_dc_get_state(bsp_input_t* const _this, bsp_input_state_t* state,
                                                uint32_t ui32_timeout);

static void bsp_input_dc_process(bsp_input_t* const _this);

/******************************************************************************
 * Function Definitions
 *******************************************************************************/

/**
 *
 * @param _this
 * @param p_cfg
 */
void
bsp_input_dc_add(bsp_input_dc_t* const _this, bsp_input_dc_cfg_t const* const p_cfg) {
  static struct input_vtbl const vtbl = {bsp_input_dc_get_state, bsp_input_dc_process};

  bsp_input_add(&_this->input, (bsp_input_cfg_t*)p_cfg);

  /* Replace the virtual table with ac input vtable */
  _this->input.vptr = &vtbl;

  _this->ui8_state = 0;
  _this->ui32_start_time = 0;
  _this->i8_last_status = !(uint8_t)p_cfg->active_state;

  /* Store the state of the line when the state is active */
  _this->ui8_active_state = (uint8_t)p_cfg->active_state;
}

/**
 *
 * @param _this
 * @param state
 * @param ui32_timeout
 * @return
 */
static bsp_input_error_t
bsp_input_dc_get_state(bsp_input_t* const _this, bsp_input_state_t* state, uint32_t ui32_timeout) {
  bsp_input_dc_t* const _me = (bsp_input_dc_t* const)_this;
  bsp_input_error_t error = BSP_INPUT_OK;
  uint32_t ui32_start;

  if (0u != ui32_timeout) {
    /* Store the tick now */
    ui32_start = bsp_tick_get();

    while (((bsp_tick_get() - ui32_start) < ui32_timeout) && (_me->input.state == BSP_INPUT_UNKNOWN_STATE)) {}

    if (BSP_INPUT_UNKNOWN_STATE == _me->input.state) {
      error = BSP_INPUT_TIMEOUT_ERROR;
    }
  }

  *state = _me->input.state;

  return error;
}

/**
 *
 * @param _this
 */
static void
bsp_input_dc_process(bsp_input_t* const _this) {
  bsp_input_dc_t* const _me = (bsp_input_dc_t*)_this;

  uint32_t now;
  uint8_t ui8_status;

  /* Read values */
  now = bsp_tick_get();

  ui8_status = (uint8_t)GPIO_PinRead(_me->input.gpio, _me->input.pin);

  /* First stage */
  if (_me->ui8_state == BSP_DC_INPUT_LINE_STATE_START) {
    /* Check if pressed */
    if (ui8_status != _me->i8_last_status) {
      /* Digital line changed, go to stage BSP_DIGITAL_IN_LINE_STATE_DEBOUNCE */
      _me->ui8_state = BSP_DC_INPUT_IN_STATE_DEBOUNCE;

      /* Save the last status */
      _me->i8_last_status = ui8_status;

      /* Save pressed time */
      _me->ui32_start_time = now;
    }
  }

  if (_me->ui8_state == BSP_DC_INPUT_IN_STATE_DEBOUNCE) {
    /* Check for debounce */
    if (ui8_status == _me->i8_last_status) {
      if ((now - _me->ui32_start_time) > _me->input.ui16_debounce_time) {
        /* Line debounce OK, Goto Normal Press */
        _me->ui8_state = BSP_DC_INPUT_LINE_STATE_START;

        /* Call function callback */
        if (ui8_status == (uint8_t)_me->ui8_active_state) {
          _me->input.state = BSP_INPUT_ACTIVE_STATE;

          /* Try to call user function */
          if (_me->input.callback != NULL) {
            _me->input.callback(&_me->input, BSP_INPUT_TO_ACTIVE_EVENT);
          }
        } else {
          _me->input.state = BSP_INPUT_INACTIVE_STATE;

          /* Try to call user function */
          if (_me->input.callback != NULL) {
            _me->input.callback(&_me->input, BSP_INPUT_TO_INACTIVE_EVENT);
          }
        }
      }
    } else if (ui8_status != _me->i8_last_status) {
      /* It was bounce, start over */
      /* Go to state BSP_DIGITAL_IN_LINE_STATE_START */
      _me->ui8_state = BSP_DC_INPUT_LINE_STATE_START;

      /* Save the last status */
      _me->i8_last_status = ui8_status;
    }
  }
}

/*************** END OF FUNCTIONS ***************************************************************************/
