SELECT l.k, r.k
FROM dsl_eq_left AS l
JOIN dsl_eq_right AS r
  ON l.k = r.k
 AND l.k = ANY (
       SELECT i.k
       FROM dsl_correlated_exists AS i
       WHERE i.k = l.k
         AND i.payload > r.k)
ORDER BY l.k, r.k;
