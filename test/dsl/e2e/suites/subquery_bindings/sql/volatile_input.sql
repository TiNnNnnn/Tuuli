SELECT o.i, (SELECT random() >= 0 FROM scalar_inner WHERE i = 2)
FROM scalar_outer o ORDER BY o.i;
