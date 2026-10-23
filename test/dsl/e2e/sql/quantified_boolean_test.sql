SELECT o.payload,
       true = ANY (SELECT i.k = 1 FROM dsl_correlated_exists AS i WHERE i.payload <= o.payload),
       true = ALL (SELECT i.k = 1 FROM dsl_correlated_exists AS i WHERE i.payload <= o.payload),
       false = ANY (SELECT i.k = 1 FROM dsl_correlated_exists AS i WHERE i.payload <= o.payload),
       false = ALL (SELECT i.k = 1 FROM dsl_correlated_exists AS i WHERE i.payload <= o.payload),
       true <> ANY (SELECT i.k = 1 FROM dsl_correlated_exists AS i WHERE i.payload <= o.payload),
       true <> ALL (SELECT i.k = 1 FROM dsl_correlated_exists AS i WHERE i.payload <= o.payload),
       false <> ANY (SELECT i.k = 1 FROM dsl_correlated_exists AS i WHERE i.payload <= o.payload),
       false <> ALL (SELECT i.k = 1 FROM dsl_correlated_exists AS i WHERE i.payload <= o.payload)
FROM dsl_correlated_exists AS o ORDER BY o.payload;
