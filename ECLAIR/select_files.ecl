#-doc="Analyze only the source file \"file1.c\""
#-source_files+={hide,"^/workspaces/EPTA-Eurocryor-DisplayPrototype/libraries/drivers/stm32f7xx_hal_driver/.*$"} # Hide everything except for file1.

#-doc="Analyze only the translation unit of main file \"file2.c\"."
#-frames+={hide,"!main(^.*(gui_manager\\.c|main_page\\.c)$)"} # Hide all frames except for file2.
