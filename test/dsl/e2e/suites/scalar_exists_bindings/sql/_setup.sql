CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE exists_outer(i int);
CREATE TABLE exists_inner(j int);
INSERT INTO exists_outer VALUES (NULL),(1),(2),(2);
INSERT INTO exists_inner VALUES (NULL),(0),(1),(1);
ANALYZE exists_outer;
ANALYZE exists_inner;
