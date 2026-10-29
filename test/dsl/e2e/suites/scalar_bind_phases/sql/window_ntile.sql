SELECT l.i, ntile((SELECT max(r.i) FROM phase_right r)) OVER (ORDER BY l.i) FROM phase_left l ORDER BY l.i;
