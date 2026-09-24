CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE equality_input(id int NOT NULL);
INSERT INTO equality_input VALUES (1),(2),(2),(3);
ANALYZE equality_input;
