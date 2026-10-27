SELECT o.id
FROM dsl_insub_outer AS o
WHERE CASE WHEN o.id < 0 THEN
        (SELECT i.payload FROM dsl_correlated_exists AS i)
      ELSE 0 END < (SELECT max(j.payload) FROM dsl_correlated_exists AS j)
ORDER BY o.id;
