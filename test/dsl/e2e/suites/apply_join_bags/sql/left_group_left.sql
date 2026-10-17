SELECT a.i, b.i
FROM (SELECT min(i) AS i FROM binding_left) a
LEFT JOIN (VALUES (2)) b(i) ON a.i = b.i
ORDER BY a.i, b.i;
