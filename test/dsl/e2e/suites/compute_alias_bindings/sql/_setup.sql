CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE compute_alias_input(i int);
INSERT INTO compute_alias_input VALUES (NULL), (0), (1), (2), (2), (3);
ANALYZE compute_alias_input;
