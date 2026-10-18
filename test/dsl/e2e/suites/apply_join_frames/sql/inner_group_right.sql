SELECT a.i, b.i FROM binding_left a
JOIN (SELECT min(i) AS i FROM binding_right) b ON a.i = b.i
ORDER BY a.i, b.i;
