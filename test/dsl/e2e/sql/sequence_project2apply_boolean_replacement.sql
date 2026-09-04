SELECT kind, id, value
FROM (
    SELECT 'exists' AS kind, o.id,
           bool_or(EXISTS (
               SELECT 1 FROM dsl_correlated_exists AS i WHERE i.k = o.id)) OVER () AS value
    FROM dsl_insub_outer AS o
    UNION ALL
    SELECT 'not_exists', o.id,
           bool_and(NOT EXISTS (
               SELECT 1 FROM dsl_correlated_exists AS i WHERE i.k = o.id)) OVER ()
    FROM dsl_insub_outer AS o
    UNION ALL
    SELECT 'any', o.id,
           bool_or(o.v < ANY (
               SELECT i.payload FROM dsl_correlated_exists AS i WHERE i.k = o.id)) OVER ()
    FROM dsl_insub_outer AS o
    UNION ALL
    SELECT 'all', o.id,
           bool_and(o.v < ALL (
               SELECT i.payload FROM dsl_correlated_exists AS i WHERE i.k = o.id)) OVER ()
    FROM dsl_insub_outer AS o
) AS lowered
ORDER BY kind, id;
