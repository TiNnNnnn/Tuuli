SELECT l.i, l.i <> ALL(SELECT r.i FROM bind_right r) FROM bind_left l ORDER BY l.i;
