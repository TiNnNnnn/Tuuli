SELECT count(*)
FROM (
    SELECT g, v
    FROM dsl_agg_outer
    GROUP BY g, v
) AS q;
