SELECT i FROM (SELECT i FROM binding_input ORDER BY i LIMIT 3) AS limited WHERE i > 0;
