SELECT
       false = ANY (SELECT i.k = 1 FROM dsl_correlated_exists AS i WHERE i.k IS NULL),
       true = ALL (SELECT i.k = 1 FROM dsl_correlated_exists AS i WHERE i.k IS NULL),
       true = ANY (SELECT i.k = 1 FROM dsl_correlated_exists AS i WHERE i.k IS NULL),
       false = ALL (SELECT i.k = 1 FROM dsl_correlated_exists AS i WHERE i.k IS NULL)
FROM dsl_insub_outer AS o WHERE o.id = 1;
