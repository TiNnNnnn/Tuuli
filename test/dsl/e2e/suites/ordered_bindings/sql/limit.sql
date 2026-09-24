SELECT (CASE WHEN i > 1 THEN i ELSE j END) + 10 AS x, label
FROM ordered_input ORDER BY x DESC NULLS LAST, label ASC LIMIT 2;
