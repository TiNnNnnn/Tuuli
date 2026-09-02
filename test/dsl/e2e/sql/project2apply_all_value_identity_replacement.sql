SELECT o.case_id,
       o.x = ALL (
         SELECT i.y
         FROM dsl_notin_inner AS i
         WHERE i.set_id = o.set_id) AS all_match
FROM dsl_notin_outer AS o
ORDER BY o.case_id;
