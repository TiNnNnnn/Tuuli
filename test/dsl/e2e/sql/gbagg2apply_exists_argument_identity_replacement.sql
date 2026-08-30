SELECT sum(CASE WHEN EXISTS (
                     SELECT 1
                     FROM dsl_correlated_exists AS i
                     WHERE i.k = o.id)
                THEN 1 ELSE 0 END) AS matched_outer_rows
FROM dsl_insub_outer AS o;
