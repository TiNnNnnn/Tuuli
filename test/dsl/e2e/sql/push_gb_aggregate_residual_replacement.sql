SELECT c.parent_id, c.id, count(c.id) AS total
FROM dsl_fk_child AS c
INNER JOIN dsl_fk_parent AS p
  ON c.parent_id = p.id AND c.id < p.payload
GROUP BY c.parent_id, c.id
ORDER BY c.id;
