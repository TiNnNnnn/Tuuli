CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE computed_result_input(i int);
INSERT INTO computed_result_input VALUES (NULL), (0), (1), (2), (2), (3);
ANALYZE computed_result_input;
