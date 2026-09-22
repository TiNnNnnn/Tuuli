CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE binding_input(i int, j int);
INSERT INTO binding_input VALUES
    (NULL,2),(0,1),(1,1),(2,NULL),(2,NULL),(3,2);
ANALYZE binding_input;
CREATE TABLE comparison_input(i int, j int, k int);
INSERT INTO comparison_input VALUES
    (NULL,NULL,NULL),(NULL,NULL,NULL),(1,1,1),(2,2,2),(2,2,2),
    (1,1,2),(1,2,1),(NULL,2,2),(2,NULL,2);
ANALYZE comparison_input;
