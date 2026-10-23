SELECT o.id,
       (SELECT (SELECT dsl_statement_flag(i.id, o.id)
                FROM dsl_insub_inner AS i WHERE i.id = anchor.id) AS flag
        FROM dsl_insub_outer AS anchor ORDER BY flag, anchor.id LIMIT 1) AS scalar_value,
       current_setting('dsl_test.flag_calls', true)::int AS calls
FROM dsl_insub_outer AS o
ORDER BY o.id;
