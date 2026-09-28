SELECT a.i FROM binding_left a
WHERE EXISTS (SELECT 1 FROM binding_right b WHERE a.i > b.i)
ORDER BY a.i NULLS FIRST;
