SELECT id, s::integer AS i4, s::bigint AS i8, id::bigint AS widened_id
FROM dsl_cast_safety
WHERE id > 1
ORDER BY id;
