SELECT o.k, sum((SELECT max(i.k) FROM dsl_eq_right i WHERE i.k<=o.k))
       OVER (ORDER BY o.k ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
FROM dsl_eq_left o ORDER BY 1,2;
