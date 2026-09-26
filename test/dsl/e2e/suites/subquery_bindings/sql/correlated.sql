SELECT o.i, (SELECT s.selected FROM scalar_inner s WHERE s.i = o.i) FROM scalar_outer o ORDER BY o.i;
