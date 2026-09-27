SELECT i FROM comparison_error
WHERE (CASE WHEN i = 0 THEN 10 / i ELSE j END) = j;
