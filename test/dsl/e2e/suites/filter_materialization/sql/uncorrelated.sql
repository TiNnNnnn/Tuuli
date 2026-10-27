SELECT a.i + 1 FROM binding_left a WHERE (SELECT min(b.i) FROM binding_right b) > 0 ORDER BY 1 NULLS FIRST;
