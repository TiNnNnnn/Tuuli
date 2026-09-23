SELECT a.label, b.label FROM select_input a LEFT JOIN select_input b
ON CASE WHEN a.i > 1 THEN a.i = b.i ELSE a.i = b.j END
ORDER BY a.label, b.label;
