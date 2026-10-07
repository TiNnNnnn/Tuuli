SELECT (SELECT min(i) FROM binding_right) AS v FROM binding_left ORDER BY v;
