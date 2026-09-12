SELECT o.k FROM dsl_eq_left o
WHERE o.k > 0 AND EXISTS (SELECT 1 FROM dsl_eq_right i WHERE i.k=o.k)
ORDER BY o.k;
