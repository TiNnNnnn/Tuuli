SELECT a.i, a.i FROM binding_left a WHERE 3 > (SELECT min(i) FROM binding_right)
ORDER BY a.i NULLS FIRST;
