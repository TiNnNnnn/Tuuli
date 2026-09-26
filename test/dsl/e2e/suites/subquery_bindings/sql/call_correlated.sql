SELECT o.i, (SELECT s.selected FROM scalar_inner s WHERE s.i = o.i) = (o.i > 0) FROM scalar_outer o ORDER BY o.i;
