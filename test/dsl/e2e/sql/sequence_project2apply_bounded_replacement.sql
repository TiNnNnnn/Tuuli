SELECT o.id,
       sum((SELECT max(i.payload)
            FROM dsl_correlated_exists AS i
            WHERE i.k = o.id)) OVER (
                ORDER BY o.id ROWS BETWEEN 1 PRECEDING AND 1 FOLLOWING) AS total
FROM dsl_insub_outer AS o
ORDER BY o.id;
