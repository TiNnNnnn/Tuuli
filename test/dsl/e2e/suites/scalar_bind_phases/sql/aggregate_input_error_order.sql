SELECT sum((SELECT r.i FROM phase_right r WHERE r.i <= l.i))
FROM phase_left l
WHERE 12 / (3 - l.i) > 0;
