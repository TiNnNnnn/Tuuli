SELECT a.i FROM binding_left a WHERE a.i < (SELECT i FROM binding_right WHERE i IS NOT NULL)
LIMIT 1;
