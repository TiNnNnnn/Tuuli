SELECT DISTINCT abs(CASE WHEN i > 1 THEN i ELSE -i END) AS x, label
FROM distinct_input ORDER BY x NULLS LAST, label;
