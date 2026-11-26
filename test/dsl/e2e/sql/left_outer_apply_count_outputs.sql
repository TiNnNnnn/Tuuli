SELECT o.id, lower_count.n, lower_count.copy_n,
       coalesce(lower_count.copy_n, 0::bigint) AS compensated_n
FROM dsl_insub_outer AS o
LEFT JOIN LATERAL (
    SELECT grouped.n, grouped.n AS copy_n
    FROM (
        SELECT count(*) AS n
        FROM dsl_correlated_exists AS i
        WHERE i.k = o.id
    ) AS grouped
) AS lower_count ON true
ORDER BY o.id;
