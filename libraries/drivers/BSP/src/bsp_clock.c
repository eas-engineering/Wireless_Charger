/******************************************************************************
 * Filename              : bsp_clock.c
 * Author                : Giulio Dalla Vecchia
 * Origin Date           : 29 December 2025
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

/** @file bsp_clock.c
 *  @brief This is the source file for doxygen comments function
 */

/*****************************************************************************
* Includes
******************************************************************************/
#include "bsp_clock.h"
#include "fsl_clock.h"
#include "fsl_spc.h"
#include "project_settings.h"

/*****************************************************************************
* Module Preprocessor Constants
******************************************************************************/

/*!< Board xtal0 frequency in Hz */
#define BOARD_BOOTCLOCKFRO96M_CORE_CLOCK 96000000U
#define BOARD_BOOTCLOCKFRO64M_CORE_CLOCK 64000000U
#define BOARD_BOOTCLOCKFRO48M_CORE_CLOCK 48000000U
#define BOARD_BOOTCLOCKFRO24M_CORE_CLOCK 24000000U
#define BOARD_BOOTCLOCKFRO12M_CORE_CLOCK 12000000U

/*!< Board xtal0 frequency in Hz */
#define BOARD_XTAL0_CLK_HZ               48000000U

/*****************************************************************************
* Module Preprocessor Macros
******************************************************************************/

/*****************************************************************************
* Module Typedefs
******************************************************************************/

/*****************************************************************************
* Function Prototypes
******************************************************************************/

static void bsp_clock_set_96MHz(void);
static void bsp_clock_set_64MHz(void);
static void bsp_clock_set_48MHz(void);
static void bsp_clock_set_24MHz(void);
static void bsp_clock_set_12MHz(void);

/*****************************************************************************
* Module Variable Definitions
******************************************************************************/

/*****************************************************************************
* Function Definitions
******************************************************************************/

/**
 * @brief This function is used to initialize the board clock settings.
 *
 * The function calls bsp_clock_set_64MHz() to set the board clock to 64MHz.
 *
 */
void
bsp_clock_init(void) {
  bsp_clock_set_64MHz();
}

/*******************************************************************************
 ******************** Configuration BOARD_BootClockFRO12M **********************
 ******************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!Configuration
name: BOARD_BootClockFRO12M
outputs:
- {id: CLK_1M_clock.outFreq, value: 1 MHz}
- {id: CPU_clock.outFreq, value: 12 MHz}
- {id: FRO_12M_clock.outFreq, value: 12 MHz}
- {id: MAIN_clock.outFreq, value: 12 MHz}
- {id: Slow_clock.outFreq, value: 3 MHz}
- {id: System_clock.outFreq, value: 12 MHz}
- {id: UTICK_clock.outFreq, value: 1 MHz}
settings:
- {id: SCGMode, value: SIRC}
- {id: FRO_HF_PERIPHERALS_EN_CFG, value: Disabled}
- {id: MRCC.FREQMEREFCLKSEL.sel, value: MRCC.aoi0_out0}
- {id: MRCC.FREQMETARGETCLKSEL.sel, value: MRCC.aoi0_out0}
- {id: MRCC.OSTIMERCLKSEL.sel, value: VBAT.CLK16K_1}
- {id: SCG.SCSSEL.sel, value: SCG.SIRC}
- {id: SCG_FIRCCSR_FIRCEN_CFG, value: Disabled}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */

/*******************************************************************************
 * Variables for BOARD_BootClockFRO12M configuration
 ******************************************************************************/
/*******************************************************************************
 * Code for BOARD_BootClockFRO12M configuration
 ******************************************************************************/
static void
bsp_clock_set_12MHz(void) {
  uint32_t coreFreq;
  spc_active_mode_core_ldo_option_t ldoOption;
  spc_sram_voltage_config_t sramOption;

  /* Get the CPU Core frequency */
  coreFreq = CLOCK_GetCoreSysClkFreq();

  /* The flow of increasing voltage and frequency */
  if (coreFreq <= BOARD_BOOTCLOCKFRO12M_CORE_CLOCK) {
    /* Set the LDO_CORE VDD regulator level */
    ldoOption.CoreLDOVoltage = kSPC_CoreLDO_MidDriveVoltage;
    ldoOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
    (void)SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldoOption);
    /* Configure Flash to support different voltage level and frequency */
    FMU0->FCTRL = (FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x0U));
    /* Specifies the operating voltage for the SRAM's read/write timing margin */
    sramOption.operateVoltage = kSPC_sramOperateAt1P0V;
    sramOption.requestVoltageUpdate = true;
    (void)SPC_SetSRAMOperateVoltage(SPC0, &sramOption);
  }

  CLOCK_SetupFRO12MClocking(); /*!< Setup FRO12M clock */

  CLOCK_AttachClk(kFRO12M_to_MAIN_CLK); /* !< Switch MAIN_CLK to FRO12M */

  /* The flow of decreasing voltage and frequency */
  if (coreFreq > BOARD_BOOTCLOCKFRO12M_CORE_CLOCK) {
    /* Configure Flash to support different voltage level and frequency */
    FMU0->FCTRL = (FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x0U));
    /* Specifies the operating voltage for the SRAM's read/write timing margin */
    sramOption.operateVoltage = kSPC_sramOperateAt1P0V;
    sramOption.requestVoltageUpdate = true;
    (void)SPC_SetSRAMOperateVoltage(SPC0, &sramOption);
    /* Set the LDO_CORE VDD regulator level */
    ldoOption.CoreLDOVoltage = kSPC_CoreLDO_MidDriveVoltage;
    ldoOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
    (void)SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldoOption);
  }

  /*!< Set up clock selectors - Attach clocks to the peripheries */

  /*!< Set up dividers */
  CLOCK_SetClockDiv(kCLOCK_DivAHBCLK, 1U); /* !< Set AHBCLKDIV divider to value 1 */

  /* Set SystemCoreClock variable */
  SystemCoreClock = BOARD_BOOTCLOCKFRO12M_CORE_CLOCK;
}

/*******************************************************************************
 ******************** Configuration BOARD_BootClockFRO24M **********************
 ******************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!Configuration
name: BOARD_BootClockFRO24M
outputs:
- {id: CLK_1M_clock.outFreq, value: 1 MHz}
- {id: CLK_48M_clock.outFreq, value: 48 MHz}
- {id: CPU_clock.outFreq, value: 24 MHz}
- {id: FRO_12M_clock.outFreq, value: 12 MHz}
- {id: FRO_HF_DIV_clock.outFreq, value: 48 MHz}
- {id: FRO_HF_clock.outFreq, value: 48 MHz}
- {id: MAIN_clock.outFreq, value: 48 MHz}
- {id: Slow_clock.outFreq, value: 6 MHz}
- {id: System_clock.outFreq, value: 24 MHz}
- {id: UTICK_clock.outFreq, value: 1 MHz}
settings:
- {id: MRCC.FREQMEREFCLKSEL.sel, value: MRCC.aoi0_out0}
- {id: MRCC.FREQMETARGETCLKSEL.sel, value: MRCC.aoi0_out0}
- {id: MRCC.OSTIMERCLKSEL.sel, value: VBAT.CLK16K_1}
- {id: SYSCON.AHBCLKDIV.scale, value: '2', locked: true}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */

/*******************************************************************************
 * Variables for BOARD_BootClockFRO24M configuration
 ******************************************************************************/
/*******************************************************************************
 * Code for BOARD_BootClockFRO24M configuration
 ******************************************************************************/
static void
bsp_clock_set_24MHz(void) {
  uint32_t coreFreq;
  spc_active_mode_core_ldo_option_t ldoOption;
  spc_sram_voltage_config_t sramOption;

  /* Get the CPU Core frequency */
  coreFreq = CLOCK_GetCoreSysClkFreq();

  /* The flow of increasing voltage and frequency */
  if (coreFreq <= BOARD_BOOTCLOCKFRO24M_CORE_CLOCK) {
    /* Set the LDO_CORE VDD regulator level */
    ldoOption.CoreLDOVoltage = kSPC_CoreLDO_MidDriveVoltage;
    ldoOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
    (void)SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldoOption);
    /* Configure Flash to support different voltage level and frequency */
    FMU0->FCTRL = (FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x0U));
    /* Specifies the operating voltage for the SRAM's read/write timing margin */
    sramOption.operateVoltage = kSPC_sramOperateAt1P0V;
    sramOption.requestVoltageUpdate = true;
    (void)SPC_SetSRAMOperateVoltage(SPC0, &sramOption);
  }

  CLOCK_SetupFROHFClocking(48000000U); /*!< Enable FRO HF(48MHz) output */

  CLOCK_SetupFRO12MClocking(); /*!< Setup FRO12M clock */

  CLOCK_AttachClk(kFRO_HF_to_MAIN_CLK); /* !< Switch MAIN_CLK to FRO_HF */

  /* The flow of decreasing voltage and frequency */
  if (coreFreq > BOARD_BOOTCLOCKFRO24M_CORE_CLOCK) {
    /* Configure Flash to support different voltage level and frequency */
    FMU0->FCTRL = (FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x0U));
    /* Specifies the operating voltage for the SRAM's read/write timing margin */
    sramOption.operateVoltage = kSPC_sramOperateAt1P0V;
    sramOption.requestVoltageUpdate = true;
    (void)SPC_SetSRAMOperateVoltage(SPC0, &sramOption);
    /* Set the LDO_CORE VDD regulator level */
    ldoOption.CoreLDOVoltage = kSPC_CoreLDO_MidDriveVoltage;
    ldoOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
    (void)SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldoOption);
  }

  /*!< Set up clock selectors - Attach clocks to the peripheries */

  /*!< Set up dividers */
  CLOCK_SetClockDiv(kCLOCK_DivAHBCLK, 2U);     /* !< Set AHBCLKDIV divider to value 2 */
  CLOCK_SetClockDiv(kCLOCK_DivFRO_HF_DIV, 1U); /* !< Set FROHFDIV divider to value 1 */

  /* Set SystemCoreClock variable */
  SystemCoreClock = BOARD_BOOTCLOCKFRO24M_CORE_CLOCK;
}

/*******************************************************************************
 ******************** Configuration BOARD_BootClockFRO48M **********************
 ******************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!Configuration
name: BOARD_BootClockFRO48M
outputs:
- {id: CLK_1M_clock.outFreq, value: 1 MHz}
- {id: CLK_48M_clock.outFreq, value: 48 MHz}
- {id: FRO_12M_clock.outFreq, value: 12 MHz}
- {id: FRO_HF_DIV_clock.outFreq, value: 48 MHz}
- {id: FRO_HF_clock.outFreq, value: 48 MHz}
- {id: UTICK_clock.outFreq, value: 1 MHz}
settings:
- {id: SCGMode, value: SOSC}
- {id: MRCC.FREQMEREFCLKSEL.sel, value: MRCC.aoi0_out0}
- {id: MRCC.FREQMETARGETCLKSEL.sel, value: MRCC.aoi0_out0}
- {id: MRCC.OSTIMERCLKSEL.sel, value: VBAT.CLK16K_1}
- {id: SCG.SCSSEL.sel, value: SCG.SOSC}
- {id: SCG_SOSCCSR_ERFES_SEL, value: CryOsc}
- {id: SCG_SOSCCSR_SOSCEN_CFG, value: Enabled}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */

/*******************************************************************************
 * Variables for BOARD_BootClockFRO48M configuration
 ******************************************************************************/
/*******************************************************************************
 * Code for BOARD_BootClockFRO48M configuration
 ******************************************************************************/
static void
bsp_clock_set_48MHz(void) {
  uint32_t coreFreq;
  spc_active_mode_core_ldo_option_t ldoOption;
  spc_sram_voltage_config_t sramOption;

  /* Get the CPU Core frequency */
  coreFreq = CLOCK_GetCoreSysClkFreq();

  /* The flow of increasing voltage and frequency */
  if (coreFreq <= BOARD_BOOTCLOCKFRO48M_CORE_CLOCK) {
    /* Set the LDO_CORE VDD regulator level */
    ldoOption.CoreLDOVoltage = kSPC_CoreLDO_MidDriveVoltage;
    ldoOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
    (void)SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldoOption);
    /* Configure Flash to support different voltage level and frequency */
    FMU0->FCTRL = (FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x0U));
    /* Specifies the operating voltage for the SRAM's read/write timing margin */
    sramOption.operateVoltage = kSPC_sramOperateAt1P0V;
    sramOption.requestVoltageUpdate = true;
    (void)SPC_SetSRAMOperateVoltage(SPC0, &sramOption);
  }

  CLOCK_SetupFROHFClocking(48000000U); /*!< Enable FRO HF(48MHz) output */

  CLOCK_SetupFRO12MClocking(); /*!< Setup FRO12M clock */

  /* The flow of decreasing voltage and frequency */
  if (coreFreq > BOARD_BOOTCLOCKFRO48M_CORE_CLOCK) {
    /* Configure Flash to support different voltage level and frequency */
    FMU0->FCTRL = (FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x0U));
    /* Specifies the operating voltage for the SRAM's read/write timing margin */
    sramOption.operateVoltage = kSPC_sramOperateAt1P0V;
    sramOption.requestVoltageUpdate = true;
    (void)SPC_SetSRAMOperateVoltage(SPC0, &sramOption);
    /* Set the LDO_CORE VDD regulator level */
    ldoOption.CoreLDOVoltage = kSPC_CoreLDO_MidDriveVoltage;
    ldoOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
    (void)SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldoOption);
  }

  /*!< Set up clock selectors - Attach clocks to the peripheries */

  /*!< Set up dividers */
  CLOCK_SetClockDiv(kCLOCK_DivFRO_HF_DIV, 1U); /* !< Set FROHFDIV divider to value 1 */

  /* Set SystemCoreClock variable */
  SystemCoreClock = BOARD_BOOTCLOCKFRO48M_CORE_CLOCK;
}

/*******************************************************************************
 ******************** Configuration BOARD_BootClockFRO64M **********************
 ******************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!Configuration
name: BOARD_BootClockFRO64M
outputs:
- {id: CLK_1M_clock.outFreq, value: 1 MHz}
- {id: CLK_48M_clock.outFreq, value: 48 MHz}
- {id: CPU_clock.outFreq, value: 64 MHz}
- {id: FRO_12M_clock.outFreq, value: 12 MHz}
- {id: FRO_HF_DIV_clock.outFreq, value: 64 MHz}
- {id: FRO_HF_clock.outFreq, value: 64 MHz}
- {id: MAIN_clock.outFreq, value: 64 MHz}
- {id: Slow_clock.outFreq, value: 16 MHz}
- {id: System_clock.outFreq, value: 64 MHz}
- {id: UTICK_clock.outFreq, value: 1 MHz}
settings:
- {id: VDD_CORE, value: voltage_1v1}
- {id: MRCC.FREQMEREFCLKSEL.sel, value: MRCC.aoi0_out0}
- {id: MRCC.FREQMETARGETCLKSEL.sel, value: MRCC.aoi0_out0}
- {id: MRCC.FROHFDIV.scale, value: '1', locked: true}
- {id: MRCC.OSTIMERCLKSEL.sel, value: VBAT.CLK16K_1}
- {id: SYSCON.AHBCLKDIV.scale, value: '1', locked: true}
sources:
- {id: SCG.FIRC.outFreq, value: 64 MHz}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */

/*******************************************************************************
 * Variables for BOARD_BootClockFRO64M configuration
 ******************************************************************************/
/*******************************************************************************
 * Code for BOARD_BootClockFRO64M configuration
 ******************************************************************************/
static void
bsp_clock_set_64MHz(void) {
  uint32_t coreFreq;
  spc_active_mode_core_ldo_option_t ldoOption;
  spc_sram_voltage_config_t sramOption;

  /* Get the CPU Core frequency */
  coreFreq = CLOCK_GetCoreSysClkFreq();

  /* The flow of increasing voltage and frequency */
  if (coreFreq <= BOARD_BOOTCLOCKFRO64M_CORE_CLOCK) {
    /* Set the LDO_CORE VDD regulator level */
    ldoOption.CoreLDOVoltage = kSPC_CoreLDO_NormalVoltage;
    ldoOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
    (void)SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldoOption);
    /* Configure Flash to support different voltage level and frequency */
    FMU0->FCTRL = (FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x1U));
    /* Specifies the operating voltage for the SRAM's read/write timing margin */
    sramOption.operateVoltage = kSPC_sramOperateAt1P1V;
    sramOption.requestVoltageUpdate = true;
    (void)SPC_SetSRAMOperateVoltage(SPC0, &sramOption);
  }

  CLOCK_SetupFROHFClocking(64000000U); /*!< Enable FRO HF(64MHz) output */

  CLOCK_SetupFRO12MClocking(); /*!< Setup FRO12M clock */

  CLOCK_AttachClk(kFRO_HF_to_MAIN_CLK); /* !< Switch MAIN_CLK to FRO_HF */

  /* The flow of decreasing voltage and frequency */
  if (coreFreq > BOARD_BOOTCLOCKFRO64M_CORE_CLOCK) {
    /* Configure Flash to support different voltage level and frequency */
    FMU0->FCTRL = (FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x1U));
    /* Specifies the operating voltage for the SRAM's read/write timing margin */
    sramOption.operateVoltage = kSPC_sramOperateAt1P1V;
    sramOption.requestVoltageUpdate = true;
    (void)SPC_SetSRAMOperateVoltage(SPC0, &sramOption);
    /* Set the LDO_CORE VDD regulator level */
    ldoOption.CoreLDOVoltage = kSPC_CoreLDO_NormalVoltage;
    ldoOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
    (void)SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldoOption);
  }

  /*!< Set up clock selectors - Attach clocks to the peripheries */

  /*!< Set up dividers */
  CLOCK_SetClockDiv(kCLOCK_DivAHBCLK, 1U);     /* !< Set AHBCLKDIV divider to value 1 */
  CLOCK_SetClockDiv(kCLOCK_DivFRO_HF_DIV, 1U); /* !< Set FROHFDIV divider to value 1 */

  /* Set SystemCoreClock variable */
  SystemCoreClock = BOARD_BOOTCLOCKFRO64M_CORE_CLOCK;
}

/*******************************************************************************
 ******************** Configuration BOARD_BootClockFRO96M **********************
 ******************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!Configuration
name: BOARD_BootClockFRO96M
called_from_default_init: true
outputs:
- {id: CLKOUT_clock.outFreq, value: 48 MHz}
- {id: CLK_1M_clock.outFreq, value: 1 MHz}
- {id: CLK_48M_clock.outFreq, value: 48 MHz}
- {id: CLK_IN_clock.outFreq, value: 48 MHz}
- {id: CPU_clock.outFreq, value: 48 MHz}
- {id: FRO_12M_clock.outFreq, value: 12 MHz}
- {id: FRO_HF_DIV_clock.outFreq, value: 96 MHz}
- {id: FRO_HF_clock.outFreq, value: 96 MHz}
- {id: MAIN_clock.outFreq, value: 48 MHz, locked: true, accuracy: '0.001'}
- {id: Slow_clock.outFreq, value: 12 MHz}
- {id: System_clock.outFreq, value: 48 MHz}
- {id: USB0_clock.outFreq, value: 48 MHz}
- {id: UTICK_clock.outFreq, value: 1 MHz}
settings:
- {id: SCGMode, value: SOSC}
- {id: VDD_CORE, value: voltage_1v1}
- {id: CLKOUTDIV_HALT, value: Enable}
- {id: MRCC.FREQMEREFCLKSEL.sel, value: MRCC.aoi0_out0}
- {id: MRCC.FREQMETARGETCLKSEL.sel, value: MRCC.aoi0_out0}
- {id: MRCC.FROHFDIV.scale, value: '1', locked: true}
- {id: MRCC.OSTIMERCLKSEL.sel, value: VBAT.CLK16K_1}
- {id: SCG.SCSSEL.sel, value: SCG.SOSC}
- {id: SCG_SOSCCSR_ERFES_SEL, value: CryOsc}
- {id: SCG_SOSCCSR_SOSCEN_CFG, value: Enabled}
- {id: SYSCON.AHBCLKDIV.scale, value: '1', locked: true}
sources:
- {id: SCG.FIRC.outFreq, value: 96 MHz}
- {id: SCG.SOSC.outFreq, value: 48 MHz, enabled: true}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */

/*******************************************************************************
 * Variables for BOARD_BootClockFRO96M configuration
 ******************************************************************************/
/*******************************************************************************
 * Code for BOARD_BootClockFRO96M configuration
 ******************************************************************************/
static void
bsp_clock_set_96MHz(void) {
  uint32_t coreFreq;
  spc_active_mode_core_ldo_option_t ldoOption;
  spc_sram_voltage_config_t sramOption;

  /* Get the CPU Core frequency */
  coreFreq = CLOCK_GetCoreSysClkFreq();

  /* The flow of increasing voltage and frequency */
  if (coreFreq <= BOARD_BOOTCLOCKFRO96M_CORE_CLOCK) {
    /* Set the LDO_CORE VDD regulator level */
    ldoOption.CoreLDOVoltage = kSPC_CoreLDO_NormalVoltage;
    ldoOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
    (void)SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldoOption);
    /* Configure Flash to support different voltage level and frequency */
    FMU0->FCTRL = (FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x1U));
    /* Specifies the operating voltage for the SRAM's read/write timing margin */
    sramOption.operateVoltage = kSPC_sramOperateAt1P1V;
    sramOption.requestVoltageUpdate = true;
    (void)SPC_SetSRAMOperateVoltage(SPC0, &sramOption);
  }

  CLOCK_SetupExtClocking(48000000U);
  CLOCK_SetSysOscMonitorMode(kSCG_SysOscMonitorDisable); /* System OSC Clock Monitor is disabled */

  CLOCK_SetupFROHFClocking(96000000U); /*!< Enable FRO HF(96MHz) output */

  CLOCK_SetupFRO12MClocking(); /*!< Setup FRO12M clock */

  CLOCK_AttachClk(kCLK_IN_to_MAIN_CLK); /* !< Switch MAIN_CLK to CLK_IN */

  /* The flow of decreasing voltage and frequency */
  if (coreFreq > BOARD_BOOTCLOCKFRO96M_CORE_CLOCK) {
    /* Configure Flash to support different voltage level and frequency */
    FMU0->FCTRL = (FMU0->FCTRL & ~((uint32_t)FMU_FCTRL_RWSC_MASK)) | (FMU_FCTRL_RWSC(0x1U));
    /* Specifies the operating voltage for the SRAM's read/write timing margin */
    sramOption.operateVoltage = kSPC_sramOperateAt1P1V;
    sramOption.requestVoltageUpdate = true;
    (void)SPC_SetSRAMOperateVoltage(SPC0, &sramOption);
    /* Set the LDO_CORE VDD regulator level */
    ldoOption.CoreLDOVoltage = kSPC_CoreLDO_NormalVoltage;
    ldoOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;
    (void)SPC_SetActiveModeCoreLDORegulatorConfig(SPC0, &ldoOption);
  }

  /*!< Set up clock selectors - Attach clocks to the peripheries */
  CLOCK_AttachClk(kCLK_IN_to_USB0);   /* !< Switch USB0 to CLK_IN */
  CLOCK_AttachClk(kCLK_IN_to_CLKOUT); /* !< Switch CLKOUT to CLK_IN */

  /*!< Set up dividers */
  CLOCK_SetClockDiv(kCLOCK_DivAHBCLK, 1U);     /* !< Set AHBCLKDIV divider to value 1 */
  CLOCK_SetClockDiv(kCLOCK_DivCLKOUT, 1U);     /* !< Set CLKOUTDIV divider to value 1 */
  CLOCK_SetClockDiv(kCLOCK_DivFRO_HF_DIV, 1U); /* !< Set FROHFDIV divider to value 1 */

  /* Set SystemCoreClock variable */
  SystemCoreClock = BOARD_BOOTCLOCKFRO96M_CORE_CLOCK;
}