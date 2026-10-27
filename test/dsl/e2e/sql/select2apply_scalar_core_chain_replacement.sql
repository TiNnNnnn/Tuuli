SELECT o.id FROM dsl_insub_outer AS o
WHERE (SELECT min(i.payload) FROM dsl_correlated_exists AS i WHERE i.k >= o.id) > 0
  AND (SELECT max(i.payload) FROM dsl_correlated_exists AS i WHERE i.k >= o.id) < 100
ORDER BY o.id;
