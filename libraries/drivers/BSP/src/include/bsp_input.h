/*****************************************************************************
 * Filename              :   bsp_input.h
 * Author                :   Giulio Dalla Vecchia
 * Origin Date           :   18 dic 2020
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

/** @file bsp_input.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef BSP_INPUT_H_
#define BSP_INPUT_H_

/*****************************************************************************
 * Includes
 ******************************************************************************/
#include <stdint.h> /* For portable types */
#include "project_settings.h"

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

/**
 * @brief  Input possible state types
 */
typedef enum {
  BSP_INPUT_INACTIVE_STATE,
  BSP_INPUT_ACTIVE_STATE,
  BSP_INPUT_UNKNOWN_STATE,
} bsp_input_state_t;

/**
 * @brief  Input possible event types
 */
typedef enum {
  BSP_INPUT_NO_EVENT,
  BSP_INPUT_TO_ACTIVE_EVENT,
  BSP_INPUT_TO_INACTIVE_EVENT,
  BSP_INPUT_MAX_EVENT,
} bsp_input_event_t;

/**
 * @brief  Input possible error types
 */
typedef enum {
  BSP_INPUT_OK,
  BSP_INPUT_TIMEOUT_ERROR,
} bsp_input_error_t;

/**
 * @brief  Input configuration typedef
 */
typedef struct {
  PORT_Type* port;
  GPIO_Type* gpio;
  uint32_t pin;
  uint32_t ui32_debounce_time_ms;
} bsp_input_cfg_t;

struct input_vtbl;

/**
 * @brief  Input instance typedef
 */
typedef struct bsp_input {
  struct input_vtbl const* vptr;
  GPIO_Type* gpio;
  uint32_t pin;
  uint16_t ui16_debounce_time;                                              /*!< Time in ms for debounce line */
  volatile bsp_input_state_t state;                                         /*!< State of the input */
  void (*callback)(struct bsp_input* const _this, bsp_input_event_t event); /*!< Callback function */
  void* p_user_data;                                                        /*!< user data object */
} bsp_input_t;

/**
 * @brief  Input virtual table
 */
struct input_vtbl {
  bsp_input_error_t (*get_state)(bsp_input_t* const _this, bsp_input_state_t* state, uint32_t ui32_timeout);
  void (*process)(bsp_input_t* const _this);
};

/*****************************************************************************
 * Module Variable Definitions
 ******************************************************************************/

/*****************************************************************************
 * Function Prototypes
 ******************************************************************************/

void bsp_input_add(bsp_input_t* const _this, bsp_input_cfg_t* const p_cfg);

void bsp_input_set_callback(bsp_input_t* const _this, void (*p_callback)(struct bsp_input* const _this, bsp_input_event_t event));

void bsp_input_set_user_data(bsp_input_t* const _this, void* const p_data);

static inline bsp_input_error_t
bsp_input_get_state(bsp_input_t* const _this, bsp_input_state_t* state, uint32_t ui32_timeout) {
  return (*_this->vptr->get_state)(_this, state, ui32_timeout);
}

/**
 * \}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*BSP_INPUT_H_*/

/*** End of File *************************************************************/
