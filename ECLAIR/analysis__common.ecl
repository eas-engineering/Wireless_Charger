# This file must not be renamed: it is referenced by analysis__cli.ecl
# and analysis__gui.ecl.
#
# The aim of this file is to define the common configuration to all analyses.

# Show active report tagging requests
-config=B.REPORT.ECB,tags=show

# Enable service to report a natural language explanation of the
# ECLAIR configuration.
-enable=B.EXPLAIN

# Enable service to report unused ECL configurations.
-enable=B.ECL

# Configuration for MISRA compliance related to the toolchain.
-eval_file=toolchain.ecl

-doc="The following header file declare the public API for the library."
-file_tag+={api:public,"^src/crc\\.h$"}

-doc="File tagged as api:public should be considered public API files."
-public_files+=api:public

-doc_begin="The violations of function reflect() are safe."
-config=MC3R1.R10.4,reports+={safe,"any_area(context(^reflect\\(.*$))"}
-doc_end

-doc_begin="printf() and fprintf() cannot be called on closed streams in the program (only stdout and stderr are used)."
-config=MC3R1.D4.13,calls={compliant, "any()", "^(printf||fprintf)\\(.*$"}
-doc_end

-doc="This configuration is a relic of the past."
-config=MC3R1.D4.13,calls+={compliant, "any()", "^old\\(.*$"}
