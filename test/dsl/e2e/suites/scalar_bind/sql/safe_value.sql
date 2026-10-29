SELECT i, (SELECT min(i) FROM demand_right) AS value FROM demand_left;
