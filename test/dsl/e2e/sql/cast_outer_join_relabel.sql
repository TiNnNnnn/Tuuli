SELECT o.id, i.id AS matched_id
FROM dsl_insub_outer AS o
LEFT JOIN dsl_cast_text AS i ON i.id = o.id
WHERE i.value::varchar IS NOT NULL
ORDER BY o.id;
