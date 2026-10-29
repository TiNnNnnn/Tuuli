SELECT l.i FROM phase_left l WHERE EXISTS(SELECT 1 FROM phase_right r WHERE r.i = 1) = ANY(SELECT r.i > 1 FROM phase_right r WHERE r.i <= l.i) ORDER BY l.i;
