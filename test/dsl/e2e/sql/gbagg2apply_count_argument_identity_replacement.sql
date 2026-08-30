SELECT sum((
         SELECT count(*)
         FROM dsl_correlated_exists AS i
         WHERE i.k = o.id)) AS matched_rows
FROM dsl_insub_outer AS o;
