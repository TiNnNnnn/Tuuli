SELECT id,
       NULLIF(NULLIF(v, 30), NULLIF(v, 20)) AS nullable_left,
       NULLIF(20, NULLIF(v, 30)) AS nullable_right,
       NULLIF(v, 30) IS DISTINCT FROM NULLIF(v, 20) AS different,
       NULLIF(v, v) IS NOT DISTINCT FROM NULLIF(id, id) AS both_null
FROM dsl_insub_outer
WHERE v > 10
ORDER BY id;
