SELECT (SELECT 1 / (min(i) - min(i)) FROM demand_right) AS value, i + 2147483647 AS suffix FROM demand_left;
