SELECT id, v::integer AS narrowed
FROM dsl_cast_safety
WHERE id > 1
ORDER BY id;
