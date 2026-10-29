CREATE EXTENSION pg_orca;
CREATE TABLE demand_left(i int);
CREATE TABLE demand_right(i int);
INSERT INTO demand_left VALUES (2);
INSERT INTO demand_right VALUES (1),(2);
ANALYZE demand_left;
ANALYZE demand_right;
CREATE SEQUENCE demand_sequence;
