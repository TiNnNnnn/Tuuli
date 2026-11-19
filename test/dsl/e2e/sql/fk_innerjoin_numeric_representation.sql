SELECT c.id
FROM dsl_numeric_fk_child AS c
JOIN dsl_numeric_fk_parent AS p ON c.k = p.k;
