SELECT (CASE WHEN i > 1 THEN i ELSE j END) + 10 AS x, label
FROM call_input ORDER BY label;
