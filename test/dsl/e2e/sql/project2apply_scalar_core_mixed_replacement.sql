SELECT o.id,
       CASE WHEN o.id < 0 THEN
         (SELECT i.payload FROM dsl_correlated_exists AS i)
       ELSE 0 END AS dormant,
       (SELECT max(j.payload) FROM dsl_correlated_exists AS j) AS high
FROM dsl_insub_outer AS o
ORDER BY o.id;
