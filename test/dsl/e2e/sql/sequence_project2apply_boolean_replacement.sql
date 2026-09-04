SELECT o.id,
       bool_or(EXISTS (
           SELECT 1 FROM dsl_correlated_exists AS i WHERE i.k = o.id)) OVER () AS has_match,
       bool_and(NOT EXISTS (
           SELECT 1 FROM dsl_correlated_exists AS i WHERE i.k = o.id)) OVER () AS all_missing,
       bool_or(o.v < ANY (
           SELECT i.payload FROM dsl_correlated_exists AS i WHERE i.k = o.id)) OVER () AS any_larger,
       bool_and(o.v < ALL (
           SELECT i.payload FROM dsl_correlated_exists AS i WHERE i.k = o.id)) OVER () AS all_larger
FROM dsl_insub_outer AS o
ORDER BY o.id;
