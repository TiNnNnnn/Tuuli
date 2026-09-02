SELECT sum(CASE WHEN o.x = ALL (
                     SELECT i.y
                     FROM dsl_notin_inner AS i
                     WHERE i.set_id = o.set_id)
                THEN 1 ELSE 0 END) AS all_match_rows
FROM dsl_notin_outer AS o;
