SELECT o.id,
       (SELECT i.label FROM dsl_typed_join_left AS i WHERE i.k = o.id) AS label,
       (SELECT i.amount FROM dsl_typed_join_left AS i WHERE i.k = o.id) AS amount
FROM dsl_insub_outer AS o
ORDER BY o.id;
