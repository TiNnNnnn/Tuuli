SELECT id, CASE WHEN (id = 2) IS TRUE THEN v / (v - v) ELSE v END AS value
FROM dsl_insub_outer
WHERE v > 20
ORDER BY id;
