SELECT a.i FROM binding_left a WHERE a.i < (SELECT min(i) FROM binding_empty);
