SELECT i + 2147483647 AS prefix, (SELECT 1 / (min(i) - min(i)) FROM demand_right) AS value FROM demand_left;
