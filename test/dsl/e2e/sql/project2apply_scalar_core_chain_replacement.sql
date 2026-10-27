SELECT o.id, o.v + 1 AS prefix,
       (SELECT max(i.payload) FROM dsl_correlated_exists AS i) + 1 AS high,
       o.v * 2 AS middle,
       (SELECT min(j.payload) FROM dsl_correlated_exists AS j) + 2 AS low
FROM dsl_insub_outer AS o
ORDER BY o.id;
