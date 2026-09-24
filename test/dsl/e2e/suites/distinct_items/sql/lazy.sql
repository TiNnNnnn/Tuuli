SELECT DISTINCT abs(CASE WHEN i = 0 THEN 0 ELSE 10 / i END) AS x, label
FROM distinct_input ORDER BY x NULLS LAST, label;
