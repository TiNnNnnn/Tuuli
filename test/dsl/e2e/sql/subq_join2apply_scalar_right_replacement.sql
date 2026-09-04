SELECT l.k, r.k
FROM dsl_eq_left AS l
JOIN dsl_eq_right AS r
  ON l.k = r.k
 AND r.k = (
       SELECT max(i.k)
       FROM dsl_correlated_exists AS i
       WHERE i.k = r.k)
ORDER BY l.k, r.k;
