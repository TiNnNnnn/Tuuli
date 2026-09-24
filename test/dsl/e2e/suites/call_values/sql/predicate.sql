SELECT label FROM call_input
WHERE (CASE WHEN i > 1 THEN i ELSE j END) = 2 ORDER BY label;
