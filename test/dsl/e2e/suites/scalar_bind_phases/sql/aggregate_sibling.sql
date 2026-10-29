SELECT sum((SELECT max(r.i) FROM phase_right r WHERE r.i <= l.i)), max(l.i) FROM phase_left l;
