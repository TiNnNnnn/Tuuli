SELECT a,b FROM set_left
WHERE CASE WHEN a = 1 THEN 10 / (a - 1) > 0 ELSE a > 0 END
UNION ALL
SELECT b,a FROM set_right WHERE b > 0;
