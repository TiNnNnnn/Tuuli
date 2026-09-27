CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE distinct_input(i int, label text);
INSERT INTO distinct_input VALUES
    (NULL,'null'),(NULL,'null'),(0,'zero'),(1,'one'),(2,'two'),(2,'two'),(3,'three');
CREATE TABLE distinct_empty(LIKE distinct_input);
ANALYZE distinct_input;
ANALYZE distinct_empty;
CREATE TABLE distinct_multi(i int, label text, extra int);
INSERT INTO distinct_multi VALUES
    (NULL,NULL,0),(NULL,NULL,1),(NULL,'x',2),
    (1,NULL,3),(1,'x',4),(1,'x',4),(1,'x',5),(2,'y',6);
ANALYZE distinct_multi;
