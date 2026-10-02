SELECT o.id FROM dsl_insub_outer o
WHERE 1/(o.id-o.id) > 0
  AND o.v < ANY (SELECT i.payload FROM dsl_correlated_exists i WHERE i.k = o.id)
ORDER BY o.id;
