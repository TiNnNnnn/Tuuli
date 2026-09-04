SELECT o.id,
       (SELECT max(i.payload)
        FROM dsl_correlated_exists AS i) AS max_payload,
       EXISTS (
         SELECT 1
         FROM dsl_correlated_exists AS i
         WHERE i.k = o.id) AS has_match,
       NOT EXISTS (
         SELECT 1
         FROM dsl_correlated_exists AS i
         WHERE i.k = o.id) AS is_missing,
       o.v < ANY (
         SELECT i.payload
         FROM dsl_correlated_exists AS i
         WHERE i.k = o.id) AS any_larger,
       o.v < ALL (
         SELECT i.payload
         FROM dsl_correlated_exists AS i
         WHERE i.k = o.id) AS all_larger
FROM dsl_insub_outer AS o
ORDER BY o.id;
