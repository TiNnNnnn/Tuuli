CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE set_left(a int, b int);
CREATE TABLE set_right(a int, b int);
INSERT INTO set_left VALUES (1,10),(1,10),(1,10),(2,NULL),(3,30),(4,40),(NULL,20);
INSERT INTO set_right VALUES (10,1),(10,1),(99,2),(30,3),(NULL,2),(20,NULL);
ANALYZE set_left;
ANALYZE set_right;
