SELECT a.i FROM binding_left a
WHERE a.i = (SELECT min(10 / (i - 2)) FROM binding_right);
