SELECT o.payload FROM dsl_correlated_exists o
WHERE NOT EXISTS (SELECT i.k FROM dsl_correlated_exists i WHERE i.k = o.k)
ORDER BY o.payload;
