SELECT o.case_id,
       (SELECT o.x = ALL (
          SELECT i.y FROM dsl_notin_inner AS i WHERE i.set_id = o.set_id)
        FROM dsl_insub_outer AS anchor WHERE anchor.id = 1 LIMIT 1) AS quantified_value
FROM dsl_notin_outer AS o
ORDER BY o.case_id;
