SELECT a.i, b.i
FROM binding_left a LEFT JOIN binding_right b ON a.i / 0 = b.i;
