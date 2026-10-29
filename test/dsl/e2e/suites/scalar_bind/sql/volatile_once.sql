SELECT count(DISTINCT value)
FROM (SELECT i, (SELECT nextval('demand_sequence') FROM demand_left LIMIT 1) AS value
      FROM demand_right) q;
