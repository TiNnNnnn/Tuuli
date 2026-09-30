SELECT o.payload, (SELECT count(*) FROM dsl_correlated_exists i
                   WHERE i.k = o.k) FROM dsl_correlated_exists o
ORDER BY o.payload;
