SELECT abs(CASE WHEN i = 0 THEN 10 / i ELSE i END) AS x
FROM call_input WHERE i = 0;
