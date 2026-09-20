SELECT a.i, b.i
FROM binding_left a FULL JOIN binding_right b ON a.i = b.i
ORDER BY a.i NULLS FIRST, b.i NULLS FIRST;
