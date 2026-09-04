SELECT o.id,
       sum((SELECT max(i.payload)
            FROM dsl_correlated_exists AS i
            WHERE i.k = o.id)) OVER (
                ORDER BY o.id GROUPS BETWEEN 1 PRECEDING AND 1 FOLLOWING
                EXCLUDE CURRENT ROW) AS groups_excluding_current,
       sum((SELECT max(i.payload)
            FROM dsl_correlated_exists AS i
            WHERE i.k = o.id)) OVER (
                ORDER BY o.id RANGE BETWEEN 1 PRECEDING AND 1 FOLLOWING
                EXCLUDE TIES) AS range_excluding_ties
FROM dsl_insub_outer AS o
ORDER BY o.id;
