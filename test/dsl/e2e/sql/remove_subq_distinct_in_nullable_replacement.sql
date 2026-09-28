SELECT o.a, o.b
FROM dsl_eq_pair_left AS o
WHERE o.b IN (SELECT DISTINCT i.b FROM dsl_eq_pair_right AS i)
ORDER BY o.a NULLS LAST, o.b NULLS LAST;
