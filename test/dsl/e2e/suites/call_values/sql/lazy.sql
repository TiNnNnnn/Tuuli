SELECT abs(CASE WHEN i = 0 THEN 0 ELSE 10 / i END) AS x, label
FROM call_input ORDER BY label;
