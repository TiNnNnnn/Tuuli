SELECT CASE WHEN i = 0 THEN 10 / i ELSE 99 END AS x, label
FROM select_input
ORDER BY label;
