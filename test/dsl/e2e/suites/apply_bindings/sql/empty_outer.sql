SELECT a.i, (SELECT 10 / (b.i - a.i) FROM binding_right b WHERE b.i = a.i LIMIT 1)
FROM binding_empty a;
