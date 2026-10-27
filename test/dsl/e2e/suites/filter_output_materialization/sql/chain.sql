SELECT a.i FROM binding_left a WHERE (SELECT min(b.i) FROM binding_right b WHERE b.i >= a.i) > 0 AND (SELECT max(b.i) FROM binding_right b WHERE b.i >= a.i) < 10 ORDER BY 1;
