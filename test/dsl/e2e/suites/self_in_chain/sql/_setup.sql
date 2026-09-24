CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE chain_input(id int PRIMARY KEY, v int);
INSERT INTO chain_input VALUES (1,10),(2,20),(3,30),(4,NULL);
ANALYZE chain_input;
CREATE TABLE chain_duplicates(id int NOT NULL, v int);
INSERT INTO chain_duplicates VALUES (1,10),(2,20),(2,25),(3,30),(4,NULL);
ANALYZE chain_duplicates;
