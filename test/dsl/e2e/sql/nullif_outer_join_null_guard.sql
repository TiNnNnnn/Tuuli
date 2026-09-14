SELECT o.id, i.id AS matched_id
FROM dsl_insub_outer AS o
LEFT JOIN dsl_insub_inner AS i ON i.id = o.id
WHERE NULLIF(o.id = 2, i.id = 0)
ORDER BY o.id;
