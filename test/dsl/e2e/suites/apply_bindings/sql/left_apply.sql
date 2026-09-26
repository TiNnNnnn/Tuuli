SELECT a.i, (SELECT b.i FROM binding_right b WHERE b.i = a.i LIMIT 1)
FROM binding_left a
ORDER BY a.i NULLS FIRST;
