SELECT id,
       NULLIF(v / (v - 20), 3) AS partial_left,
       NULLIF(v, v / (v - 20)) AS partial_right,
       (v / (v - 20)) IS DISTINCT FROM 3 AS different_left,
       3 IS DISTINCT FROM (v / (v - 20)) AS different_right
FROM dsl_insub_outer
WHERE v > 20
ORDER BY id;
