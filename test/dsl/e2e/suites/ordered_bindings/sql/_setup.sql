CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE ordered_input(i int, j int, label text);
INSERT INTO ordered_input VALUES
    (NULL,2,'null'),(0,1,'zero'),(1,1,'one'),(2,NULL,'two'),(2,NULL,'two'),
    (3,2,'three'),(NULL,NULL,'missing');
ANALYZE ordered_input;
