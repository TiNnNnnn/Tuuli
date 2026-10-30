SELECT sum((l.i <> ALL(SELECT r.i FROM bind_empty r))::int) FROM bind_left l;
