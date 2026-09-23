SELECT label FROM select_input
WHERE CASE WHEN i > 1 THEN j > 1 ELSE i = 0 END
ORDER BY label;
