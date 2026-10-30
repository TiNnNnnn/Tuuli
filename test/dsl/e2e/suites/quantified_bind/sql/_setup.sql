CREATE EXTENSION pg_orca;
CREATE TABLE bind_left(i int);
CREATE TABLE bind_right(i int);
CREATE TABLE bind_empty(i int);
INSERT INTO bind_left VALUES (1),(2),(3);
INSERT INTO bind_right VALUES (1),(2),(2),(NULL);
ANALYZE bind_left; ANALYZE bind_right; ANALYZE bind_empty;
