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
#include "bsp_button.h"
#include "bsp_config.h"
#include "bsp_timer.h"
#include "bsp_tick.h"

/******************************************************************************
 * Module Preprocessor Constants
 *******************************************************************************/

/* Button states */
#define BUTTON_STATE_START       (uint8_t)0U
#define BUTTON_STATE_PRESSED     (uint8_t)1U
#define BUTTON_STATE_WAITRELEASE (uint8_t)2U

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

static bsp_button_t* bsp_button_pool[BSP_KEYPAD_NUM_OF_BTN];

static uint32_t ui32_button_num;

/******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static void bsp_timer_callback(struct bsp_timer const* const _this);

static void bsp_button_process(bsp_button_t* const _this);

/******************************************************************************
 * Function Definitions
 *******************************************************************************/

/**
 *
 * @param _this
 * @param p_cfg
 */
void
bsp_button_add(bsp_button_t* const _this, bsp_button_cfg_t* p_cfg) {
  static bsp_timer_t* bsp_timer_ptr = NULL;

  assert(NULL != _this);
  assert(NULL != p_cfg);
  assert(ui32_button_num < (sizeof(bsp_button_pool) / sizeof(bsp_button_pool[0])));

  /* Initialize button structure */
  _this->pf_callback = NULL;
  _this->State = BUTTON_STATE_START;
  _this->PressNormalTime = p_cfg->ui32_normal_press_time;
  _this->PressLongTime = p_cfg->ui32_long_press_time;
  _this->eventNow = BSP_INPUT_NO_EVENT;

  /* Config pointer to the input object */
  _this->p_input = p_cfg->p_input;

  /* Set the user input data useful to control input handler */
  bsp_input_set_user_data(_this->p_input, (void*)_this);

  /* Store ID value */
  _this->id = p_cfg->id;

  __disable_irq();
  bsp_button_pool[ui32_button_num] = _this;
  ++ui32_button_num;
  __enable_irq();

  if (NULL == bsp_timer_ptr) {
    bsp_timer_ptr = &timer_process;
    bsp_timer_add(bsp_timer_ptr, bsp_timer_callback);
    bsp_timer_arm(bsp_timer_ptr, 10u, 10u);
  }
}

/**
 *
 * @param _this
 * @param state
 * @return
 */
bsp_button_error_t
bsp_button_get_state(bsp_button_t* const _this, bsp_button_state_t* state) {
  bsp_button_error_t error = BSP_BUTTON_OK;
  bsp_input_state_t input_state;

  if (bsp_input_get_state(_this->p_input, &input_state, 300) == BSP_INPUT_OK) {
    if (input_state == BSP_INPUT_ACTIVE_STATE) {
      *state = BSP_BUTTON_PRESSED_STATE;
    } else {
      *state = BSP_BUTTON_RELEASED_STATE;
    }
  } else {
    error = BSP_BUTTON_UNKNOWN_STATE;
  }

  return error;
}

/**
 *
 * @param _this
 * @param pf
 */
void
bsp_button_set_callback(bsp_button_t* const _this, bsp_button_callback_t const pf) {
  _this->pf_callback = pf;
}

/**
 * @brief Set the long press time for a button
 *
 * @param[in] _this pointer to the button object
 * @param[in] time the time in ms for long press
 *
 * @details
 * This function sets the long press time for a specific button. The time is
 * in milliseconds and should be set > 0.
 *
 * @pre
 * - The button object must exist
 * - The time must be > 0
 *
 * @post
 * - The long press time for the button is set to the given value
 *
 * @note
 * - The long press time is used to detect long press events
 */
void
bsp_button_set_long_press_time(bsp_button_t* const _this, uint16_t time) {
  assert(NULL != _this);
  assert(time > 0);

  _this->PressLongTime = time;
}

/**
 *
 * @param _this
 */
static void
bsp_timer_callback(struct bsp_timer const* const _this) {
  (void)_this;

  for (uint32_t i = 0; i < ui32_button_num; ++i) {
    bsp_button_process(bsp_button_pool[i]);
  }
}

/**
 *
 * @param _this
 */
static void
bsp_button_process(bsp_button_t* const _this) {
  uint32_t now;
  bsp_input_state_t input_state;

  /* Process the digital input link to the button */
  (void)bsp_input_get_state(_this->p_input, &input_state, 0u);

  if (BSP_INPUT_UNKNOWN_STATE == input_state) {
    return;
  }

  /* Read values */
  now = bsp_tick_get();

  /* First stage */
  if (_this->State == BUTTON_STATE_START) {
    /* Check if pressed */
    if (BSP_INPUT_ACTIVE_STATE == input_state) {
      /* Button pressed OK, call function */
      if (_this->pf_callback != NULL) {
        /* Call function callback */
        _this->pf_callback(_this, BSP_BUTTON_ONPRESSED_EVENT);
      }

      /* Button pressed, go to stage BUTTON_STATE_START */
      _this->State = BUTTON_STATE_PRESSED;

      /* Save pressed time */
      _this->StartTime = now;
    }
  }

  if (_this->State == BUTTON_STATE_PRESSED) {
    /* Button still pressed */
    /* Check for long press */
    if (BSP_INPUT_ACTIVE_STATE == input_state) {
      if ((now - _this->StartTime) > _this->PressLongTime) {
        /* Button pressed OK, call function */
        if (_this->pf_callback != NULL) {
          /* Call function callback */
          _this->pf_callback(_this, BSP_BUTTON_LONG_PRESS_EVENT);
        }

        /* Go to stage BUTTON_STATE_WAITRELEASE */
        _this->State = BUTTON_STATE_WAITRELEASE;

        /* Save pressed time */
        _this->StartTime = now;
      }
    } else if (input_state == BSP_INPUT_INACTIVE_STATE) {
      /* Not pressed */
      if ((now - _this->StartTime) > _this->PressNormalTime) {
        /* Button pressed OK, call function */
        if (_this->pf_callback != NULL) {
          /* Call function callback */
          _this->pf_callback(_this, BSP_BUTTON_NORMAL_PRESS_EVENT);
        }

        /* Go to stage BUTTON_STATE_WAITRELEASE */
        _this->State = BUTTON_STATE_WAITRELEASE;
      } else {
        /* Go to state BUTTON_STATE_START */
        _this->State = BUTTON_STATE_START;
      }
    } else {
      /* Go to state BUTTON_STATE_START */
      _this->State = BUTTON_STATE_START;
    }
  }

  if (_this->State == BUTTON_STATE_WAITRELEASE) {
    /* Wait till button released */
    if (input_state == BSP_INPUT_INACTIVE_STATE) {
      /* Button pressed OK, call function */
      if (_this->pf_callback != NULL) {
        /* Call function callback */
        _this->pf_callback(_this, BSP_BUTTON_RELEASE_EVENT);
      }

      /* Go to stage 0 again */
      _this->State = BUTTON_STATE_START;
    } else if (input_state == BSP_INPUT_ACTIVE_STATE) {
      if ((now - _this->StartTime) > _this->PressLongTime) {
        /* Button pressed OK, call function */
        if (_this->pf_callback != NULL) {
          /* Call function callback */
          _this->pf_callback(_this, BSP_BUTTON_LONG_PRESS_EVENT);
        }

        /* Save pressed time */
        _this->StartTime = now;
      }
    }
  }
}

/*************** END OF FUNCTIONS ***************************************************************************/
