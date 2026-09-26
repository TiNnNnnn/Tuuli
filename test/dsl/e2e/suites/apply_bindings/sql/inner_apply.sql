SELECT a.i FROM binding_left a
WHERE a.i = (SELECT b.i FROM binding_right b WHERE b.i = a.i LIMIT 1)
ORDER BY a.i NULLS FIRST;
