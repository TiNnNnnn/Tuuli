SELECT id, value::integer AS parsed
FROM dsl_cast_safety
WHERE id > 1
ORDER BY id;
