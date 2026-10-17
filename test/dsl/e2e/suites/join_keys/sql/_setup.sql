CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE key_parent(k int PRIMARY KEY, extra int NOT NULL);
CREATE TABLE key_child(k int NOT NULL REFERENCES key_parent(k), extra int NOT NULL);
INSERT INTO key_parent VALUES (1,10),(2,20),(3,30);
INSERT INTO key_child VALUES (1,10),(2,99),(3,30);
ANALYZE key_parent;
ANALYZE key_child;
