SELECT sum((
         SELECT max(i.payload)
         FROM dsl_correlated_exists AS i
         WHERE i.k OPERATOR(dsl_nonkey.=) o.id)) AS total_payload
FROM dsl_insub_outer AS o;
