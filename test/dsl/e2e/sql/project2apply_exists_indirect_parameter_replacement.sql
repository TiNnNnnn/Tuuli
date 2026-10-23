SELECT o.id,
       (SELECT EXISTS (SELECT 1 FROM dsl_correlated_exists AS i WHERE i.k = o.id)
        FROM dsl_insub_outer AS anchor WHERE anchor.id = o.id LIMIT 1) AS subquery_value
FROM dsl_insub_outer AS o
ORDER BY o.id;
