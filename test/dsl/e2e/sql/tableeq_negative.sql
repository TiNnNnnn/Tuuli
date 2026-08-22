SELECT id, v
FROM dsl_insub_outer o
WHERE id IN (SELECT id FROM dsl_insub_inner i1)
  AND id IN (SELECT id FROM dsl_insub_outer i2)
ORDER BY id;

