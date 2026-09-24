SELECT (CASE WHEN i > 1 THEN i ELSE j END) + 10 AS x, label
FROM ordered_input WHERE i < -100 ORDER BY x DESC NULLS LAST, label ASC;
