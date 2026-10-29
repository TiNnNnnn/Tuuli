SELECT i, EXISTS (SELECT 1 FROM demand_right WHERE i > 0) AS value FROM demand_left;
