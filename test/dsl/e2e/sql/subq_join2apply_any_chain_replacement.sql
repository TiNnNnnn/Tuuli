SELECT l.k, r.k, p.a
FROM dsl_eq_left AS l
JOIN dsl_eq_right AS r ON l.k = r.k
JOIN dsl_eq_pair_right AS p
  ON p.a = l.k
 AND p.a = ANY (
       SELECT i.a
       FROM dsl_eq_pair_left AS i
       WHERE i.b = r.k)
ORDER BY l.k, r.k, p.a;
