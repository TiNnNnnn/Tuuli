SELECT a.i FROM binding_left a
WHERE a.i = (SELECT min(i) FROM binding_empty HAVING min(i) IS NOT NULL)
ORDER BY a.i NULLS FIRST;
