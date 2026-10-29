SELECT o.i FROM demand_left o WHERE (SELECT r.i FROM demand_right r WHERE r.i=o.i+10) > 0;
