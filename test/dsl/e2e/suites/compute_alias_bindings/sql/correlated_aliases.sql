SELECT o.i, (SELECT (SELECT o.i)) AS copied
FROM compute_alias_input AS o ORDER BY o.i NULLS FIRST;
