SELECT sum((l.i <> ALL(SELECT r.i FROM bind_right r))::int) FROM bind_left l;
