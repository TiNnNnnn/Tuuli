CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE dsl_correlated_exists(k int, payload int NOT NULL);
INSERT INTO dsl_correlated_exists VALUES (1,10),(NULL,20),(2,30),(2,40);
ANALYZE dsl_correlated_exists;
