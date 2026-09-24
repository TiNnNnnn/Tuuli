SELECT DISTINCT abs(CASE WHEN i > 1 THEN i ELSE -i END) AS x
FROM distinct_empty;
