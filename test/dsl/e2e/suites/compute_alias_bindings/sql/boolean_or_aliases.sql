SELECT i, x OR NOT y AS flag
FROM (SELECT i, l AS x, r AS y FROM compute_boolean_input) AS aliases
ORDER BY i;
