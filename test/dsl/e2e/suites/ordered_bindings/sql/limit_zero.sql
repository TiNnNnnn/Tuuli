SELECT (CASE WHEN i = 0 THEN 100 / i ELSE j END) + 10 AS x, label
FROM ordered_input ORDER BY x DESC NULLS LAST, label ASC LIMIT 0;
