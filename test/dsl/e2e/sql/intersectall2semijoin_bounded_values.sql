SELECT coalesce(k::text,'<null>') FROM (SELECT k FROM (VALUES (1), (1), (NULL::int), (2)) AS l(k)
INTERSECT ALL
SELECT k FROM (VALUES (NULL::int), (1), (3)) AS r(k)
) AS v ORDER BY k NULLS LAST;
