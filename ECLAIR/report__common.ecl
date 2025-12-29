# eclair_report

# This file must not be renamed: it is referenced by report__cli.ecl
# and report__gui.ecl.
#
# The aim of this file is to define the eclair_report configuration
# common to all (GUI-driven or CLI-driven) analyses.

# Full output in pure text format.
-full_txt=join_paths(output_dir,"txt")
# Output metrics for use with spreadsheet applications (if enabled).
-metrics_tab=join_paths(output_dir,"metrics")
# Output reports for use with spreadsheet applications
-reports_tab=join_paths(output_dir,"reports")

# Show only first area in reports
-first_area
# Full output in ODT format.
-full_odt=join_paths(output_dir,"odt")
# Full output in DOC format.
-full_doc=join_paths(output_dir,"doc")
