SELECT
       true = ANY (SELECT i.k = 1 FROM dsl_correlated_exists AS i WHERE i.payload < 0),
       true = ALL (SELECT i.k = 1 FROM dsl_correlated_exists AS i WHERE i.payload < 0),
       false = ANY (SELECT i.k = 1 FROM dsl_correlated_exists AS i WHERE i.payload < 0),
       false = ALL (SELECT i.k = 1 FROM dsl_correlated_exists AS i WHERE i.payload < 0)
FROM dsl_insub_outer AS o WHERE o.id = 1;
