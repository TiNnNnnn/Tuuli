SELECT o.k,
       EXISTS (SELECT 1 FROM dsl_nullable_unique i WHERE i.a=o.k),
       NOT EXISTS (SELECT 1 FROM dsl_nullable_unique i WHERE i.a=o.k),
       o.k IN (SELECT a FROM dsl_nullable_unique),
       o.k <> ALL (SELECT a FROM dsl_nullable_unique),
       (SELECT max(i.a) FROM dsl_nullable_unique i WHERE i.a<=o.k)
FROM dsl_eq_left o ORDER BY o.k;
