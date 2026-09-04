SELECT c.parent_id, count(c.id) AS total
FROM dsl_fk_child AS c
INNER JOIN dsl_fk_parent AS p ON c.parent_id = p.id
GROUP BY c.parent_id
ORDER BY c.parent_id;
