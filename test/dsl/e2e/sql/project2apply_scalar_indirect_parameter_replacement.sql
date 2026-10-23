SELECT o.id,
       (SELECT (SELECT max(i.payload)
                FROM dsl_correlated_exists AS i WHERE i.k = o.id)
        FROM dsl_insub_outer AS anchor WHERE anchor.id = o.id LIMIT 1) AS scalar_value
FROM dsl_insub_outer AS o
ORDER BY o.id;
