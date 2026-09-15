CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE binding_input(i int, j int);
INSERT INTO binding_input VALUES
    (NULL,2),(0,1),(1,1),(2,NULL),(2,NULL),(3,2);
ANALYZE binding_input;
