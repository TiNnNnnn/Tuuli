SELECT sum(CASE WHEN o.v < ANY (
                     SELECT i.payload
                     FROM dsl_correlated_exists AS i
                     WHERE i.k = o.id)
                THEN 1 ELSE 0 END) AS matched_outer_rows
FROM dsl_insub_outer AS o;
