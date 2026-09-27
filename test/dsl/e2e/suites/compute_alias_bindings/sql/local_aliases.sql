SELECT q.b, q.c FROM (
    SELECT p.b, p.b AS c FROM (SELECT i AS b FROM compute_alias_input) AS p
) AS q ORDER BY q.b NULLS FIRST;
