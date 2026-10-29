CREATE EXTENSION pg_orca;
CREATE TABLE phase_left(i int);
CREATE TABLE phase_right(i int);
INSERT INTO phase_left VALUES (0),(1),(2),(3);
INSERT INTO phase_right VALUES (1),(2),(2);
ANALYZE phase_left;
ANALYZE phase_right;
