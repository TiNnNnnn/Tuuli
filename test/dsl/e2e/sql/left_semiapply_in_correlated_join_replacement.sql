SELECT o.k
FROM dsl_eq_left AS o
WHERE o.k IN (
    SELECT i.k
    FROM dsl_eq_right AS i
    JOIN dsl_eq_right AS j
      ON i.k = j.k
     AND j.k = o.k)
ORDER BY o.k;
