CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE compute_input(i int, j int, label text);
INSERT INTO compute_input VALUES
    (NULL,2,'null'),(0,1,'zero'),(1,1,'one'),(2,NULL,'two'),(2,NULL,'two'),(3,2,'three');
ANALYZE compute_input;
