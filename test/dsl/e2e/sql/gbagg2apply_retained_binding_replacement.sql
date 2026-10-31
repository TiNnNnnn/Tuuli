SELECT max((
           SELECT max(i.payload)
           FROM dsl_correlated_exists AS i
           WHERE i.k = o.id) + (
           SELECT min(i.payload)
           FROM dsl_correlated_exists AS i
           WHERE i.k = o.id)) AS combined_max
FROM dsl_insub_outer AS o;
