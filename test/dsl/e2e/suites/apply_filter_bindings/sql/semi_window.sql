SELECT o.payload FROM dsl_correlated_exists o
WHERE EXISTS (SELECT max(i.payload) OVER (PARTITION BY i.k)
              FROM dsl_correlated_exists i WHERE i.k = o.k)
ORDER BY o.payload;
