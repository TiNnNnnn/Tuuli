SELECT l.i, sum(CASE WHEN l.i < 0 THEN (SELECT r.i FROM phase_right r) ELSE l.i END) OVER () FROM phase_left l ORDER BY l.i;
