SELECT o.id,
       sum((SELECT max(i.payload)
            FROM dsl_correlated_exists AS i
            WHERE i.k = o.id)) OVER (
                ROWS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING) AS total
FROM dsl_insub_outer AS o
ORDER BY o.id;
