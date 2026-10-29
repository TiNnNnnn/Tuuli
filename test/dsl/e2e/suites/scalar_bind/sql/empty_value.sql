SELECT i, (SELECT i FROM demand_right WHERE i > 10) AS value FROM demand_left;
