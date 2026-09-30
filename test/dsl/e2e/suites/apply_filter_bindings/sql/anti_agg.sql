SELECT o.payload FROM dsl_correlated_exists o
WHERE NOT EXISTS (SELECT max(i.payload) FROM dsl_correlated_exists i
                  WHERE i.k = o.k GROUP BY i.payload HAVING max(i.payload) > 0)
ORDER BY o.payload;
