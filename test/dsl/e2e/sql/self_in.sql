SELECT id, v
FROM dsl_insub_outer o
WHERE id IN (SELECT id FROM dsl_insub_outer i)
  AND v > 10
ORDER BY id;

