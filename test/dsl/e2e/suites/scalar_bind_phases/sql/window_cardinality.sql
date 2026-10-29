SELECT l.i, sum((SELECT r.i FROM phase_right r WHERE r.i <= l.i)) OVER () FROM phase_left l ORDER BY l.i;
