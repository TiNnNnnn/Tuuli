SELECT o.id
FROM dsl_insub_outer AS o
WHERE (o.id #=# 0) IS NOT NULL
  AND o.v = (
    SELECT v FROM (VALUES (1,10)) AS scalar_input(id,v)
    WHERE id = o.id)
ORDER BY o.id;
