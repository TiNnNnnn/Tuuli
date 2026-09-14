SELECT id, CASE WHEN (id = 2) IS NOT FALSE THEN v ELSE NULL END AS value
FROM dsl_insub_outer
WHERE v > 10
ORDER BY id;
