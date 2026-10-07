SELECT a.i FROM binding_left a WHERE a.i = (SELECT i FROM binding_right)
ORDER BY a.i NULLS FIRST;
