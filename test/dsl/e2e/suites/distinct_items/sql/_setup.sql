CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE distinct_input(i int, label text);
INSERT INTO distinct_input VALUES
    (NULL,'null'),(NULL,'null'),(0,'zero'),(1,'one'),(2,'two'),(2,'two'),(3,'three');
CREATE TABLE distinct_empty(LIKE distinct_input);
ANALYZE distinct_input;
ANALYZE distinct_empty;
