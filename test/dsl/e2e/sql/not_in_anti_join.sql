SELECT count(*)
FROM dsl_notin_tag t
JOIN dsl_fk_child c ON c.id = t.id
JOIN dsl_fk_parent p ON p.id = c.parent_id
WHERE p.id NOT IN (
    SELECT i.y
    FROM dsl_notin_inner i
    WHERE i.set_id = 3
);
