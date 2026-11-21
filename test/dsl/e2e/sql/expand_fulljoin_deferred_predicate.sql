SELECT l.k, r.k
FROM dsl_eq_left l FULL JOIN dsl_eq_right r
  ON l.k = r.k AND 10 / (l.k + r.k - 4) > 0
ORDER BY l.k, r.k;
