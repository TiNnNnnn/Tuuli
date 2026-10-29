SELECT l.i, EXISTS(SELECT 1 FROM phase_right r WHERE r.i < 0), NOT EXISTS(SELECT 1 FROM phase_right r WHERE r.i < 0) FROM phase_left l ORDER BY l.i;
