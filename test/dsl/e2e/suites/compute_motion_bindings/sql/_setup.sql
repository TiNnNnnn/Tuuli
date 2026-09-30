CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE dsl_insub_outer(id int PRIMARY KEY, v int);
INSERT INTO dsl_insub_outer VALUES (1,10),(2,20),(3,30),(4,NULL);
ANALYZE dsl_insub_outer;
CREATE TABLE dsl_cast_safety(id int PRIMARY KEY, v bigint, value text);
INSERT INTO dsl_cast_safety VALUES
    (1,9223372036854775807,'not an integer'),(2,42,'42'),(3,NULL,NULL);
ANALYZE dsl_cast_safety;
