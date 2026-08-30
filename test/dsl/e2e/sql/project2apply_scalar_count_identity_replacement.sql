SELECT o.id,
       (SELECT count(*)
        FROM dsl_correlated_exists AS i
        WHERE i.k = o.id) AS match_count
FROM dsl_insub_outer AS o
ORDER BY o.id;
