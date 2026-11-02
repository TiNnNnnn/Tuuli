CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE dsl_eq_left(k int);
CREATE TABLE dsl_eq_right(k int);
INSERT INTO dsl_eq_left VALUES (1),(1),(2),(3);
INSERT INTO dsl_eq_right VALUES (NULL),(1),(2),(2);
ANALYZE dsl_eq_left;
ANALYZE dsl_eq_right;
