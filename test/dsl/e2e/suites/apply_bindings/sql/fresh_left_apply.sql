SELECT a.i, b.i FROM binding_left a
LEFT JOIN (SELECT min(i) AS i FROM binding_right) b ON a.i = b.i
ORDER BY a.i NULLS FIRST;
