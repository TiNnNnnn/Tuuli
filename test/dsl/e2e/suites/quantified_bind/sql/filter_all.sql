SELECT l.i FROM bind_left l WHERE l.i <> ALL(SELECT r.i FROM bind_right r) ORDER BY l.i;
