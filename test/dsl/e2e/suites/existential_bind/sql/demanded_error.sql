SELECT EXISTS(SELECT 1 FROM phase_right r WHERE 1/(r.i-r.i) > l.i) FROM phase_left l ORDER BY l.i;
