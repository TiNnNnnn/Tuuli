SELECT sum((l.i = ANY(SELECT r.i FROM bind_empty r))::int) FROM bind_left l;
