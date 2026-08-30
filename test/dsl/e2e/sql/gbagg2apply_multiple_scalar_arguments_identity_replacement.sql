SELECT sum(coalesce((
             SELECT max(i.payload)
             FROM dsl_correlated_exists AS i
             WHERE i.k = o.id), 0) + (
             SELECT count(*)
             FROM dsl_correlated_exists AS i
             WHERE i.k = o.id)) AS combined_total
FROM dsl_insub_outer AS o;
