-- DISTINCT(EXCEPT ALL) is not EXCEPT DISTINCT, including for NULL rows.
SELECT 'except_all', count(*) FROM (
  VALUES (1), (1), (NULL::int), (NULL::int)
  EXCEPT ALL VALUES (1), (NULL::int)
) s
UNION ALL
SELECT 'except_distinct', count(*) FROM (
  VALUES (1), (1), (NULL::int), (NULL::int)
  EXCEPT VALUES (1), (NULL::int)
) s
UNION ALL
SELECT 'distinct_of_except_all', count(*) FROM (
  SELECT DISTINCT * FROM (
    VALUES (1), (1), (NULL::int), (NULL::int)
    EXCEPT ALL VALUES (1), (NULL::int)
  ) d
) s
ORDER BY 1;
