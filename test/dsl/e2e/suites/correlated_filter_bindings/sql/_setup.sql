CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE correlation_outer(i int);
CREATE TABLE correlation_inner(j int);
INSERT INTO correlation_outer VALUES (NULL),(1),(2),(2);
INSERT INTO correlation_inner VALUES (NULL),(0),(1),(1);
ANALYZE correlation_outer;
ANALYZE correlation_inner;
