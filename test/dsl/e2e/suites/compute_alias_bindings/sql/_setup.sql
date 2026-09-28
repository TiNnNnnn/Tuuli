CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE compute_alias_input(i int);
INSERT INTO compute_alias_input VALUES (NULL), (0), (1), (2), (2), (3);
ANALYZE compute_alias_input;
CREATE TABLE compute_boolean_input(i int, l boolean, r boolean);
INSERT INTO compute_boolean_input VALUES
    (1, FALSE, FALSE), (2, FALSE, TRUE), (3, FALSE, NULL),
    (4, TRUE, FALSE), (5, TRUE, TRUE), (6, TRUE, NULL),
    (7, NULL, FALSE), (8, NULL, TRUE), (9, NULL, NULL),
    (4, TRUE, FALSE);
ANALYZE compute_boolean_input;
