SELECT o.id
FROM dsl_insub_outer o
WHERE EXISTS (
    SELECT 1 FROM dsl_insub_outer i WHERE i.id <= o.id OFFSET 1)
  AND NOT EXISTS (
    SELECT 1 FROM dsl_insub_outer j WHERE j.id > o.id OFFSET 1)
ORDER BY o.id;
