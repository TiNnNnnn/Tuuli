SELECT l.i, l.i = ANY(SELECT r.i FROM bind_empty r) FROM bind_left l ORDER BY l.i;
