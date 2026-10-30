SELECT l.i, l.i = 1 OR l.i = ANY(SELECT r.i / (l.i - 1) FROM bind_right r WHERE r.i IS NOT NULL) FROM bind_left l ORDER BY l.i;
