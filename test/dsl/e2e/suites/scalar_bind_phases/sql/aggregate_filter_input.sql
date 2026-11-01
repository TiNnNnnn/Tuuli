SELECT sum(l.i + (SELECT r.i FROM phase_right r WHERE r.i = l.i AND r.i < 2)) FROM phase_left l;
