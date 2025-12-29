# NEEDED: load ECLAIR settings common to all CLI-driven analyses.
-eval_file=analysis__cli.ecl

-enable=B.REPORT.ECB
-config=B.REPORT.ECB,output=join_paths(data_dir,"FRAME.@FRAME@.ecb")
-config=B.REPORT.ECB,preprocessed=show
-config=B.REPORT.ECB,tags=show
-config=B.REPORT.ECB,macros=10

-enable=B.EXPLAIN

-enable=B.BUGFIND
-eval_file=analysis__common.ecl

-reports+={hide,all_exp_external}
