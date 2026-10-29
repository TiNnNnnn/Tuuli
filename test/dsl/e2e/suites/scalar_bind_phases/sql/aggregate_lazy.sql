SELECT sum(CASE WHEN l.i < 0 THEN (SELECT r.i FROM phase_right r) ELSE l.i END) FROM phase_left l;
