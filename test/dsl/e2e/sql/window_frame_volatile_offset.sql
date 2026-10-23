SELECT id, sum(v) OVER (ORDER BY id ROWS BETWEEN
       (CASE WHEN dsl_statement_flag(1,1) THEN 1 ELSE 0 END) PRECEDING AND CURRENT ROW),
       dsl_statement_flag_calls()
FROM dsl_insub_outer ORDER BY id;
