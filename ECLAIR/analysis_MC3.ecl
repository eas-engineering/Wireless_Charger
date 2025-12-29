# This file must be named analyze_<RULESET>.ecl, where <RULESET> is the first
# argument of analyze.bat.
#
# The aim of this file is to define the analysis configuration for <RULESET>.
#
# The essential portions of this file are marked with "# NEEDED":
# they may be adapted of course.

# NEEDED: load ECLAIR settings common to all (CLI-driven or IDE-driven) analyses.
-eval_file=analysis__cli.ecl

# Enable MISRA C:2012 Revision 1 Amendment 2 guidelines' checkers.
#-enable=MC3R1

# Enable LTLM (Language/Toolchain/Library Misuse) group --> harmful situations that should always be avoided
-enable=MC3R1.D4.11,MC3R1.D4.13,MC3R1.R1.1,MC3R1.R1.3,MC3R1.R5.1,MC3R1.R5.2,MC3R1.R6.1,MC3R1.R8.6,MC3R1.R8.10,MC3R1.R9.1,MC3R1.R9.4,MC3R1.R13.1,MC3R1.R13.2,MC3R1.R17.4,MC3R1.R17.5,MC3R1.R18.1,MC3R1.R18.2,MC3R1.R18.3,MC3R1.R18.6,MC3R1.R19.1,MC3R1.R20.2,MC3R1.R20.3,MC3R1.R20.4,MC3R1.R20.6,MC3R1.R20.11,MC3R1.R20.13,MC3R1.R20.14,MC3R1.R21.1,MC3R1.R21.2,MC3R1.R21.13,MC3R1.R21.14,MC3R1.R21.17,MC3R1.R21.18,MC3R1.R21.19,MC3R1.R21.20,MC3R1.R22.2,MC3R1.R22.4,MC3R1.R22.6,MC3R1.R22.8,MC3R1.R22.10

# Enable DEVM (Developer Misreading/Mistyping/Misunderstanding) --> reduce the risk of misleading other rules
-enable=MC3R1.D4.4,MC3R1.D4.5,MC3R1.D4.9,MC3R1.R3.1,MC3R1.R3.2,MC3R1.R4.1,MC3R1.R4.2,MC3R1.R5.3,MC3R1.R5.5,MC3R1.R5.6,MC3R1.R5.7,MC3R1.R5.8,MC3R1.R5.9,MC3R1.R6.2,MC3R1.R7.1,MC3R1.R7.2,MC3R1.R7.3,MC3R1.R8.1,MC3R1.R8.3,MC3R1.R8.5,MC3R1.R8.8,MC3R1.R8.11,MC3R1.R8.12,MC3R1.R9.2,MC3R1.R9.3,MC3R1.R9.5,MC3R1.R12.1,MC3R1.R12.4,MC3R1.R12.5,MC3R1.R13.3,MC3R1.R13.4,MC3R1.R13.6,MC3R1.R14.3,MC3R1.R15.6,MC3R1.R17.8,MC3R1.R20.7,MC3R1.R20.8,MC3R1.R20.9

# Enable PORT for portability
-enable=MC3R1.D4.6,MC3R1.R1.2,MC3R1.R6.1,MC3R1.R22.5

# Enable HTDR (Hard To Do Right) --> inexperienced programmer are in team
-enable=MC3R1.D4.12,MC3R1.R8.14,MC3R1.R12.3,MC3R1.R14.1,MC3R1.R16.3,MC3R1.R17.1,MC3R1.R17.2,MC3R1.R17.6,MC3R1.R18.5,MC3R1.R18.7,MC3R1.R18.8,MC3R1.R19.2,MC3R1.R20.1,MC3R1.R20.5,MC3R1.R20.10,MC3R1.R20.12,MC3R1.R21.3,MC3R1.R21.4,MC3R1.R21.5,MC3R1.R21.6,MC3R1.R21.8,MC3R1.R21.9,MC3R1.R21.10,MC3R1.R21.12,MC3R1.R21.16,MC3R1.R21.21,MC3R1.R22.3,MC3R1.R22.5

# Enable ENMO (Encapsulation/Modularization) --> maintainability and the possibilty of reusing code for other project
-enable=MC3R1.D4.3,MC3R1.D4.8,MC3R1.R8.7,MC3R1.R8.9

#Enable TYPM (Typing)
-enable=MC3R1.R10.1,MC3R1.R10.2,MC3R1.R10.3,MC3R1.R10.4,MC3R1.R10.5,MC3R1.R10.6,MC3R1.R10.7,MC3R1.R10.8,MC3R1.R12.2,MC3R1.R14.4,MC3R1.R16.7,MC3R1.R21.15

# Load other ECLAIR configrations.
-eval_file=select_files.ecl

#-frame_selector={middleware,"main(^libraries/drivers/cmsis_core/.*$)"}
#-frame_selector={middleware,"main(^libraries/drivers/cmsis_device_f7/.*$)"}
#-frame_selector={middleware,"main(^libraries/drivers/stm32f7xx_hal_driver/.*$)"}
-frame_selector={middleware,"main(^libraries/middlewares/.*$)"}
-doc="Libraries are not under MISRA compliance."
-frames+={hide,middleware}

#
# PER CONFIGURARE UNA MACRO COME SICURA
#
#-doc="EDF_EVT_CAST denotes safe down casts."
#-config=MC3R1.R11.3,reports+={safe,"all_area(all_loc(macro(^EDF_EVT_CAST$)))"}

#
# PER CONFIGURARE UNA CARTELLA CHE NON SARA' VISITATA DA ECLAIR
#
#-doc="Not meant to comply with any rule."
#-source_files+={hide,"^D:/project/CIMBALI/COLLAUDO_928345_V01/library/middleware/freertos/.*$"}
#
#-frame_selector={freertos,"main(^D:/project/CIMBALI/COLLAUDO_928345_V01/library/middleware/freertos/.*$)"}
#-doc="Libraries are not under MISRA compliance."
#-frames+={hide,freertos}

#-frame_selector={middleware,"main(^D:/project/COMEM/DGA/DGA_APPLICATION/library/middleware/.*$)"}
#-doc="Libraries are not under MISRA compliance."
#-frames+={hide,middleware}

#
#   ESEMPIO
#
#-frame_selector={sys,"main(^D:/project/COMEM/DGA/DGA_APPLICATION/project/board/.*$)"}
#-frame_selector={sys,"main(^D:/project/COMEM/DGA/DGA_APPLICATION/project/startup/.*$)"}
#-frame_selector={sys,"main(^D:/project/COMEM/DGA/DGA_APPLICATION/project/CMSIS/.*$)"}
#-doc="Libraries are not under MISRA compliance."
#-frames+={hide,sys}