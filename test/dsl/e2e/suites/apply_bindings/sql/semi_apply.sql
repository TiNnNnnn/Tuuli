SELECT a.i FROM binding_left a
WHERE EXISTS (SELECT 1 FROM binding_right b WHERE b.i = a.i LIMIT 1)
ORDER BY a.i NULLS FIRST;
