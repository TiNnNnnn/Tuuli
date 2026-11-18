SELECT o.id FROM dsl_insub_outer AS o
WHERE o.id NOT IN (SELECT i.id FROM dsl_insub_inner AS i)
ORDER BY o.id;
