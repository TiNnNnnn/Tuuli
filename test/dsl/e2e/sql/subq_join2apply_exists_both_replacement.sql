SELECT l.k, r.k
FROM dsl_eq_left AS l
JOIN dsl_eq_right AS r
  ON l.k = r.k
 AND EXISTS (
       SELECT 1
       FROM dsl_correlated_exists AS i
       WHERE i.k = l.k
         AND i.payload > r.k)
ORDER BY l.k, r.k;
