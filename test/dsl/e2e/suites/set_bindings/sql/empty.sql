SELECT a,b FROM set_left WHERE a > 99
UNION ALL
SELECT b,a FROM set_right WHERE b > 99;
