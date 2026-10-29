SELECT sum((SELECT r.i FROM phase_right r)) FROM phase_left l WHERE l.i < 0;
