SELECT l.i, sum((SELECT max(r.i) FROM phase_right r WHERE r.i <= l.i)) OVER (ORDER BY l.i ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) FROM phase_left l ORDER BY l.i;
