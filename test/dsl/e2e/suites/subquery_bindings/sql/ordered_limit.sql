SELECT o.i, (SELECT s.selected FROM scalar_inner s WHERE s.i < o.i ORDER BY s.i DESC LIMIT 1)
FROM scalar_outer o ORDER BY o.i;
