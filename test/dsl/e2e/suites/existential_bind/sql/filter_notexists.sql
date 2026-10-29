SELECT l.i FROM phase_left l WHERE l.i = 0 OR NOT EXISTS(SELECT 1 FROM phase_right r WHERE r.i = l.i) ORDER BY l.i;
