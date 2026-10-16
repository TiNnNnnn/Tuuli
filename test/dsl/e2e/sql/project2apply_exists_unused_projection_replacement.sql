SELECT o.id,
       EXISTS (
         SELECT 1 / (i.payload - i.payload)
         FROM dsl_correlated_exists AS i
         WHERE i.k = o.id) AS has_match,
       NOT EXISTS (
         SELECT 1 / (i.payload - i.payload)
         FROM dsl_correlated_exists AS i
         WHERE i.k = o.id) AS no_match,
       EXISTS (
         SELECT 1 / (i.payload - i.payload) + random()
         FROM dsl_correlated_exists AS i
         WHERE i.k = o.id) AS volatile_has_match,
       NOT EXISTS (
         SELECT 1 / (i.payload - i.payload) + random()
         FROM dsl_correlated_exists AS i
         WHERE i.k = o.id) AS volatile_no_match
FROM dsl_insub_outer AS o
ORDER BY o.id;
