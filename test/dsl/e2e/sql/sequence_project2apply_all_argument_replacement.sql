SELECT o.id, sum(CASE WHEN o.v < ALL (
                     SELECT i.payload
                     FROM dsl_correlated_exists AS i
                     WHERE i.k = o.id)
                THEN 1 ELSE 0 END) OVER (ROWS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING) AS matched_outer_rows
FROM dsl_insub_outer AS o
ORDER BY o.id;
