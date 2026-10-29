SELECT o.i, (SELECT r.i FROM demand_right r WHERE r.i=o.i) AS first_value, (SELECT s.i FROM demand_right s WHERE s.i=o.i+1) AS second_value FROM demand_left o;
