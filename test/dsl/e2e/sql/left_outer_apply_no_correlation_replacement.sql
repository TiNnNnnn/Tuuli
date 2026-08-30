SELECT o.k, (SELECT max(i.k) FROM dsl_eq_right AS i)
FROM dsl_eq_left AS o
ORDER BY o.k;
