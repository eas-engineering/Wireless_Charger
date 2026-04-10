/*****************************************************************************
 * Filename              :   bsp_input_dc.h
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

/** @file bsp_input_dc.h
 *  @brief This module handles the doxygen comments.
 *
 *  This is the header file for the definition of doxygen comments function.
 */

#ifndef BSP_INPUT_DC_H_
#define BSP_INPUT_DC_H_

/*****************************************************************************
 * Includes
 ******************************************************************************/
#include <stdint.h> /* For portable types */
#include "bsp_input.h"
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
 * @brief  Input active state typedef
 */
typedef enum {
  BSP_INPUT_DC_ACTIVE_LOW_STATE = 0x0,
  BSP_INPUT_DC_ACTIVE_HIGH_STATE,
  BSP_INPUT_DC_MAX_ACTIVE_STATE,
} bsp_input_active_state_t;

/**
 * @brief  Input configuration typedef
 */
typedef struct {
  PORT_Type* port;
  GPIO_Type* gpio;
  uint32_t pin;
  uint32_t ui32_debounce_time_ms;        /* !! Do not modified the previous variable order */
  bsp_input_active_state_t active_state; /* The state when the line is active */
} bsp_input_dc_cfg_t;

/**
 * @brief  DC Input instance typedef
 */
typedef struct {
  bsp_input_t input;

  volatile uint32_t ui32_start_time; /*!< Time when line changed */
  volatile int8_t i8_last_status;    /*!< Line status on last check */
  volatile uint8_t ui8_state;        /*!< Current line state */
  uint8_t ui8_active_state;          /*!< Store the line state when active */
} bsp_input_dc_t;

/*****************************************************************************
 * Module Variable Definitions
 ******************************************************************************/

/*****************************************************************************
 * Function Prototypes
 ******************************************************************************/

void bsp_input_dc_add(bsp_input_dc_t* const _this, bsp_input_dc_cfg_t const* const p_cfg);

/**
 * \}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*BSP_INPUT_DC_H_*/

/*** End of File *************************************************************/
