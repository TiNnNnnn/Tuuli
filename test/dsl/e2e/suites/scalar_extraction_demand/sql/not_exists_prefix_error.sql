SELECT i + 2147483647 AS prefix, NOT EXISTS (SELECT 1 FROM demand_right WHERE 1 / (i - i) > 0) AS value FROM demand_left;
