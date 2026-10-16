SELECT count(*), dsl_statement_flag_calls()
FROM (
  SELECT DISTINCT c.parent_id
  FROM dsl_fk_child AS c
  INNER JOIN dsl_fk_parent AS p
    ON c.parent_id = p.id AND dsl_statement_flag(c.parent_id, p.id)
) AS q;
