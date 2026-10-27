SELECT o.id,
       (SELECT (SELECT i.id FROM dsl_insub_inner AS i WHERE i.id = o.id)
        FROM dsl_insub_outer AS anchor WHERE anchor.id = o.id LIMIT 1) AS scalar_value
FROM dsl_insub_outer AS o
ORDER BY o.id;
