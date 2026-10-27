SELECT a.i + 1 FROM binding_left a WHERE (SELECT min(b.i) FROM binding_right b WHERE b.i=a.i) > 0 ORDER BY 1 NULLS FIRST;
