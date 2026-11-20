SELECT k, row_number() OVER (PARTITION BY k) AS pos
FROM (VALUES (1), (1), (NULL::int)) AS l(k)
UNION
SELECT k, row_number() OVER (PARTITION BY k) AS pos
FROM (VALUES (1), (2), (NULL::int)) AS r(k)
ORDER BY k NULLS LAST, pos;
