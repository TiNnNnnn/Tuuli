SELECT o.id
FROM dsl_insub_outer AS o
WHERE o.v = (SELECT v.x FROM (VALUES (1), (2), (3)) AS v(x) WHERE v.x > o.id)
LIMIT 1;
