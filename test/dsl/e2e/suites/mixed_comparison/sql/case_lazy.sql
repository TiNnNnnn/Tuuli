SELECT (CASE WHEN i = 0 THEN j ELSE 10 / i END) = j AS matched
FROM comparison_error;
