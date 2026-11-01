SELECT sum(12 / (3 - l.i)),
       sum((SELECT r.i FROM phase_right r WHERE r.i <= l.i))
FROM phase_left l;
