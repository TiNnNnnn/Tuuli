SELECT a.i FROM binding_left a
WHERE 10 / (a.i - 2) = (SELECT min(i) FROM binding_right);
