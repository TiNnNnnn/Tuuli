SELECT sum((SELECT r.i FROM phase_right r WHERE r.i <= l.i)),
       sum(12 / l.i)
FROM phase_left l;
