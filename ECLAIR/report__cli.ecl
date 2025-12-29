# eclair_report

# This file must not be renamed: it is referenced by analyze.sh.
#
# The aim of this file is to define the ECLAIR report configuration common
# to all CLI-based analyses.
#
# The essential portions of this file are marked with "# NEEDED":
# they may be adapted of course.

# NEEDED: set the variable for the output directory from the environment
# variable.
-setq=output_dir,getenv("ECLAIR_OUTPUT_DIR")

# NEEDED: load the eclair_report settings common to all (GUI-driven or
# CLI-driven) analyses.
-eval_file=report__common.ecl
