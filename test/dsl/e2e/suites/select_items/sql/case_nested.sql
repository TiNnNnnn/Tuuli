SELECT CASE WHEN i > 1 THEN CASE WHEN j > 1 THEN i ELSE -1 END ELSE j END AS x, label
FROM select_input
ORDER BY label;
