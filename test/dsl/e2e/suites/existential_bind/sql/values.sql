SELECT l.i, EXISTS(SELECT 1 FROM phase_right r WHERE r.i <= l.i), NOT EXISTS(SELECT 1 FROM phase_right r WHERE r.i <= l.i) FROM phase_left l ORDER BY l.i;
