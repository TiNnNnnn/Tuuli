SELECT l.k, r.k
FROM dsl_eq_left AS l
JOIN dsl_eq_right AS r
  ON l.k = r.k
 AND l.k = (
       SELECT max(i.k)
       FROM dsl_correlated_exists AS i
       WHERE i.k = l.k
         AND i.payload > r.k)
 AND r.k = (
       SELECT max(j.k)
       FROM dsl_correlated_exists AS j
       WHERE j.k = r.k
         AND j.payload > l.k)
ORDER BY l.k, r.k;
