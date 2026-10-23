SELECT o.case_id,
       false = ALL (
         SELECT i.y = 1 FROM dsl_notin_inner AS i
         WHERE i.set_id = o.set_id) AS quantified_value
FROM dsl_notin_outer AS o
ORDER BY o.case_id;
