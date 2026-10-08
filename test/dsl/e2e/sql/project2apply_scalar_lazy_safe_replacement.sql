SELECT o.id,
       CASE WHEN o.id < 2 THEN
         (SELECT v FROM (VALUES (2,10)) AS scalar_input(id,v) WHERE id = 2)
       ELSE 0 END AS value
FROM dsl_insub_outer AS o
ORDER BY o.id;
