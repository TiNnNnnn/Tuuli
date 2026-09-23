SELECT label FROM select_input
WHERE CASE WHEN i > 1 THEN CASE WHEN j > 1 THEN i > 2 ELSE false END ELSE i = 0 END
ORDER BY label;
