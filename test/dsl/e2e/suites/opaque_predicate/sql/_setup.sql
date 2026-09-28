CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE opaque_outer(i int);
CREATE TABLE opaque_inner(j int);
INSERT INTO opaque_outer VALUES (NULL),(0),(1),(2),(2);
INSERT INTO opaque_inner VALUES (NULL),(0),(1),(1);
ANALYZE opaque_outer;
ANALYZE opaque_inner;
