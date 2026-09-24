SELECT call_volatile(CASE WHEN i > 1 THEN i ELSE j END) AS x, label
FROM call_input ORDER BY label;
