SELECT l.i, NOT EXISTS(SELECT 1 FROM phase_right r WHERE r.i = l.i) AS found FROM phase_left l ORDER BY l.i;
