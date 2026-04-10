/******************************************************************************
* Filename              :   bsp_timer.c
* Author                :   Giulio Dalla Vecchia
* Origin Date           :   01 apr 2021
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

/** @file bsp_timer.c
 *  @brief This is the source file for doxygen comments function
 */
/******************************************************************************
* Includes
*******************************************************************************/
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include "bsp_timer.h"
#include "cmsis_gcc.h"

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

static bsp_timer_t* timer_ptr[10]; /* all Timer in the application */
static uint32_t ui32_timer_num;    /* current number of Timer */

/******************************************************************************
* Function Prototypes
*******************************************************************************/

/******************************************************************************
* Function Definitions
*******************************************************************************/

/**
 *
 * @param _this
 * @param fn
 */
void
bsp_timer_add(bsp_timer_t* const _this, bsp_timer_callback_fn_t fn) {
  assert(NULL != _this);
  assert(NULL != fn);
  assert(ui32_timer_num < sizeof(timer_ptr) / sizeof(timer_ptr[0]));

  _this->ui32_interval = 0u;
  _this->ui32_timeout = 0u;
  _this->callback_fn = fn;

  __disable_irq();
  timer_ptr[ui32_timer_num] = _this;
  ++ui32_timer_num;
  __enable_irq();
}

/**
 *
 * @param _this
 * @param ui32_timeout
 * @param ui32_interval
 */
void
bsp_timer_arm(bsp_timer_t* const _this, uint32_t ui32_timeout, uint32_t ui32_interval) {
  assert(NULL != _this);

  __disable_irq();
  _this->ui32_interval = ui32_timeout;
  _this->ui32_timeout = ui32_interval;
  __enable_irq();
}

/**
 *
 * @param _this
 */
void
bsp_timer_disarm(bsp_timer_t* const _this) {
  assert(NULL != _this);

  __disable_irq();
  _this->ui32_timeout = 0u;
  __enable_irq();
}

/**
 *
 */
void
bsp_timer_tick(void) {
  for (uint32_t i = 0U; i < ui32_timer_num; ++i) {
    bsp_timer_t* const t = timer_ptr[i];
    assert(t);                       /* TimeEvent instance must be registered */
    if (t->ui32_timeout > 0u) {      /* is this TimeEvent armed? */
      if (--t->ui32_timeout == 0u) { /* is it expiring now? */
        if (NULL != t->callback_fn) {
          (*t->callback_fn)(t);
        }
        t->ui32_timeout = t->ui32_interval; /* rearm or disarm (one-shot) */
      }
    }
  }
}

/*************** END OF FUNCTIONS ***************************************************************************/
