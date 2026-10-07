SELECT a.i FROM binding_left a WHERE a.i = (SELECT min(i) FROM binding_right)
ORDER BY a.i NULLS FIRST;
