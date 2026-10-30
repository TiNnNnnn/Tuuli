SELECT l.i FROM bind_left l WHERE l.i <> ALL(SELECT r.i FROM bind_empty r) ORDER BY l.i;
