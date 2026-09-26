CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE binding_left(i int);
CREATE TABLE binding_right(i int);
CREATE TABLE binding_empty(i int);
INSERT INTO binding_left VALUES (NULL),(0),(1),(2),(2);
INSERT INTO binding_right VALUES (NULL),(2),(2),(3);
ANALYZE binding_left;
ANALYZE binding_right;
ANALYZE binding_empty;
