SELECT o.k FROM dsl_eq_left AS o WHERE EXISTS (SELECT i.k FROM dsl_eq_right AS i WHERE 100 / (i.k + 10) > 0) ORDER BY o.k;
