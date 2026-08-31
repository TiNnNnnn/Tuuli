SELECT o.case_id
FROM dsl_notin_outer AS o
WHERE o.x NOT IN (
    SELECT i.y
    FROM dsl_notin_inner AS i
    JOIN dsl_notin_tag AS t
      ON t.id = i.y
     AND i.set_id = o.set_id)
ORDER BY o.case_id;
