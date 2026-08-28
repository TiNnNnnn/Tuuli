SELECT k
FROM dsl_eq_left AS o
WHERE k IN (
    SELECT DISTINCT k
    FROM dsl_eq_left AS i)
ORDER BY k;
