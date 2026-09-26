SELECT a.i, (SELECT b.i FROM binding_right b WHERE b.i = a.i)
FROM binding_left a;
