# This file must not be renamed: it is referenced by analysis_*.ecl.
#
# The aim of this file is to define the ECLAIR configuration common to all
# CLI-driven analyses.
#
# The essential portions of this file are marked with "# NEEDED":
# they may be adapted of course.

# NEEDED: load the ECLAIR settings common to all (GUI-driven or
# CLI-driven) analyses.
-eval_file=analysis__common.ecl

# NEEDED: set the project name from the environment variable.
-project_name=getenv("ECLAIR_PROJECT_NAME")

# NEEDED: set the project root from the environment variable.
-project_root=getenv("ECLAIR_PROJECT_ROOT")

# NEEDED: set the variable for data directory from the environment variable.
-setq=data_dir,getenv("ECLAIR_DATA_DIR")

# NEEDED: enable binary output of ECLAIR reports.
-enable=B.REPORT.ECB

# NEEDED: set the binary output destination.
-config=B.REPORT.ECB,output=join_paths(data_dir,"FRAME.@FRAME@.ecb")

# Reports are integrated with preprocessed source.
-config=B.REPORT.ECB,preprocessed=show
# Reports are integrated with tagging origin and documentation.
-config=B.REPORT.ECB,tags=show
# Reports are integrated with up to the specified levels of macro expansion.
-config=B.REPORT.ECB,macros=10

# Hides all reports that have all areas external to project root tree.
-reports+={hide,all_exp_external}

# Set analysis to verbose.
-verbose
