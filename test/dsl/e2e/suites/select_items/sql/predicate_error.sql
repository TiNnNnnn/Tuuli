SELECT label FROM select_input
WHERE CASE WHEN i = 0 THEN 10 / i > 1 ELSE true END
ORDER BY label;
