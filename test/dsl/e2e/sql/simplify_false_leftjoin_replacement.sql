SELECT c.id, p.payload
FROM dsl_fk_child AS c
LEFT JOIN dsl_fk_parent AS p ON FALSE
ORDER BY c.id;
