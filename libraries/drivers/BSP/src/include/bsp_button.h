/*****************************************************************************
 * Filename              :   bsp_button.h
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

/** @file bsp_button.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef BSP_BUTTON_H_
#define BSP_BUTTON_H_

/*****************************************************************************
 * Includes
 ******************************************************************************/
#include "bsp_input.h"

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
 * @brief  Button possible event types
 */
typedef enum {
  BSP_BUTTON_NO_EVENT = 0x00,
  BSP_BUTTON_FIRST_BOOT_PRESS_EVENT, /*!< Special event for the first press after boot */
  BSP_BUTTON_NORMAL_PRESS_EVENT, /*!< Normal press type, released */
  BSP_BUTTON_LONG_PRESS_EVENT,   /*!< Long press type */
  BSP_BUTTON_ONPRESSED_EVENT,    /*!< Button pressed */
  BSP_BUTTON_RELEASE_EVENT,      /*!< Button release */
  BSP_BUTTON_MAX_EVENT,
} bsp_button_event_t;

/**
 * @brief  Button possible event types
 */
typedef enum {
  BSP_BUTTON_PRESSED_STATE = 0x00, /*!< Button pressed */
  BSP_BUTTON_RELEASED_STATE,       /*!< Button released */
  BSP_BUTTON_MAX_STATE,
} bsp_button_state_t;

/**
 * @brief  Input possible error types
 */
typedef enum {
  BSP_BUTTON_OK,
  BSP_BUTTON_UNKNOWN_STATE,
  BSP_BUTTON_MAX_ERROR,
} bsp_button_error_t;

struct bsp_button;

typedef void (*bsp_button_callback_t)(struct bsp_button* const _this, bsp_button_event_t event);

typedef int32_t button_id_t;

/**
 * @brief  Button instance typedef
 */
typedef struct bsp_button {
  bsp_input_t* p_input;
  bsp_input_event_t eventNow;        /* Input event */
  bsp_button_callback_t pf_callback; /* Button function handler */
  volatile uint32_t StartTime;       /* Time when button was pressed */
  volatile uint8_t State;            /* Current button state */
  uint16_t PressNormalTime;          /* Time in ms for normal press for button */
  uint16_t PressLongTime;            /* Time in ms for long press for button */
  button_id_t id;
} bsp_button_t;

/**
 * @brief  Button configuration typedef
 */
typedef struct {
  bsp_input_t* p_input;
  uint32_t ui32_normal_press_time;
  uint32_t ui32_long_press_time;
  button_id_t id;
} bsp_button_cfg_t;

/*****************************************************************************
 * Module Variable Definitions
 ******************************************************************************/

/*****************************************************************************
 * Function Prototypes
 ******************************************************************************/

void bsp_button_add(bsp_button_t* const _this, bsp_button_cfg_t* p_cfg);

bsp_button_error_t bsp_button_get_state(bsp_button_t* const _this, bsp_button_state_t* state);

void bsp_button_set_callback(bsp_button_t* const _this, bsp_button_callback_t const pf);

void bsp_button_set_long_press_time(bsp_button_t* const _this, uint16_t time);

/**
 * \}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*BSP_BUTTON_H_*/

/*** End of File *************************************************************/
