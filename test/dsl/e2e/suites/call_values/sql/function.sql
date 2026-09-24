SELECT abs(CASE WHEN i > 1 THEN i ELSE -i END) AS x, label
FROM call_input ORDER BY label;
