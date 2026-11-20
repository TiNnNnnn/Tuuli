SELECT a, a, b FROM set_left
UNION ALL
SELECT b, a, a FROM set_right
ORDER BY 1, 2, 3;
