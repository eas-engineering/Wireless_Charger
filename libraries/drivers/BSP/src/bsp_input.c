/******************************************************************************
 * Filename              :   bsp_input.c
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

/** @file bsp_input.c
 *  @brief This is the source file for doxygen comments function
 */

/******************************************************************************
 * Includes
 *******************************************************************************/
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include "bsp_config.h"
#include "bsp_input.h"
#include "bsp_timer.h"

/******************************************************************************
 * Module Preprocessor Constants
 *******************************************************************************/

/******************************************************************************
 * Module Preprocessor Macros
 *******************************************************************************/

/******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static bsp_timer_t timer_process;

static bsp_input_t* bsp_input_line_pool[BSP_KEYPAD_NUM_OF_BTN];

static uint32_t ui32_input_line_num;

/******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static bsp_input_error_t bsp_input_get_state_(bsp_input_t* const _this, bsp_input_state_t* state, uint32_t ui32_timeout);

static void bsp_timer_callback(struct bsp_timer const* const _this);

static void bsp_input_process_(bsp_input_t* const _this);

/******************************************************************************
 * Function Definitions
 *******************************************************************************/

/**
 *
 * @param _this
 * @param p_cfg
 */
void
bsp_input_add(bsp_input_t* const _this, bsp_input_cfg_t* const p_cfg) {
  static struct input_vtbl const vtbl = {bsp_input_get_state_, bsp_input_process_};
  static bsp_timer_t* bsp_timer_ptr = NULL;

  assert(ui32_input_line_num < (sizeof(bsp_input_line_pool) / sizeof(bsp_input_line_pool[0])));

  _this->gpio = p_cfg->gpio;
  _this->pin = p_cfg->pin;
  _this->ui16_debounce_time = p_cfg->ui32_debounce_time_ms;

  /* Set initial value to MAX_STATE, it means that the input state
   * is not available yet */
  _this->state = BSP_INPUT_UNKNOWN_STATE;

  _this->vptr = &vtbl;

  __disable_irq();
  bsp_input_line_pool[ui32_input_line_num] = _this;
  ++ui32_input_line_num;
  __enable_irq();

  if (NULL == bsp_timer_ptr) {
    bsp_timer_ptr = &timer_process;
    bsp_timer_add(bsp_timer_ptr, bsp_timer_callback);
    bsp_timer_arm(bsp_timer_ptr, 5u, 5u);
  }
}

/**
 *
 * @param _this
 * @param p_callback
 */
void
bsp_input_set_callback(bsp_input_t* const _this, void (*p_callback)(struct bsp_input* const _this, bsp_input_event_t event)) {
  _this->callback = p_callback;
}

/**
 *
 * @param _this
 * @param p_data
 */
void
bsp_input_set_user_data(bsp_input_t* const _this, void* const p_data) {
  _this->p_user_data = p_data;
}

/**
 *
 * @param _this
 * @param state
 * @param ui32_timeout
 * @return
 */
static inline bsp_input_error_t
bsp_input_get_state_(bsp_input_t* const _this, bsp_input_state_t* state, uint32_t ui32_timeout) {
  return BSP_INPUT_TIMEOUT_ERROR;
}

/**
 *
 * @param _this
 */
static void
bsp_timer_callback(struct bsp_timer const* const _this) {
  bsp_input_t* input_ptr;

  (void)_this;

  for (uint32_t i = 0; i < ui32_input_line_num; ++i) {
    input_ptr = bsp_input_line_pool[i];
    assert(NULL != input_ptr);
    if (NULL != input_ptr->vptr->process) {
      (*input_ptr->vptr->process)(input_ptr);
    }
  }
}

/**
 *
 * @param _this
 */
static inline void
bsp_input_process_(bsp_input_t* const _this) {
  /* Nothing to do */
}

/*************** END OF FUNCTIONS ***************************************************************************/
