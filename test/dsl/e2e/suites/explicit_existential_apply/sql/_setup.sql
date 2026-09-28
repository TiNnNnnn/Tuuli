CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE apply_outer(i int);
CREATE TABLE apply_inner(j int);
CREATE TABLE apply_empty(j int);
INSERT INTO apply_outer VALUES (NULL),(1),(2),(2);
INSERT INTO apply_inner VALUES (NULL),(0),(1),(1);
ANALYZE apply_outer;
ANALYZE apply_inner;
ANALYZE apply_empty;
