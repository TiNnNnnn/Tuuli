SELECT a.i, b.i FROM binding_left a JOIN binding_right b ON a.i = abs(b.i)
ORDER BY a.i, b.i;
