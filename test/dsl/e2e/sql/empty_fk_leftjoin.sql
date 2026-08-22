SELECT c.id, p.id
FROM (
    SELECT *
    FROM dsl_fk_child
    WHERE FALSE
) AS c
LEFT JOIN dsl_fk_parent AS p ON c.parent_id = p.id;

