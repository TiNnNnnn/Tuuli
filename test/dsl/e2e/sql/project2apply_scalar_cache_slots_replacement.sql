SELECT o.id,
       (SELECT NULLIF(dsl_statement_flag(i.id, i.id), false) FROM dsl_insub_inner AS i WHERE i.id = 1) AS first_value,
       (SELECT dsl_statement_flag(j.id, j.id) FROM dsl_insub_inner AS j WHERE j.id = 1) AS second_value,
       current_setting('dsl_test.flag_calls', true)::int AS calls
FROM dsl_insub_outer AS o
ORDER BY o.id;
