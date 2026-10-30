SELECT l.i FROM bind_left l WHERE l.i = ANY(SELECT r.i FROM bind_right r) ORDER BY l.i;
