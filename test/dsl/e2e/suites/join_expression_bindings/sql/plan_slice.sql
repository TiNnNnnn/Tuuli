SELECT a.i, b.i
FROM binding_left a LEFT JOIN binding_right b ON a.i = b.i OR a.i + 1 = b.i;
