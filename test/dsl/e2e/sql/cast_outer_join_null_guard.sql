SELECT o.id, i.id AS matched_id
FROM dsl_insub_outer AS o
LEFT JOIN dsl_insub_inner AS i ON i.id = o.id
WHERE i.id::bigint::boolean
ORDER BY o.id;
