CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE qp_outer(i int, v int);
CREATE TABLE qp_inner(v int);
INSERT INTO qp_outer VALUES (1,NULL),(2,0),(3,1),(4,2);
INSERT INTO qp_inner VALUES (NULL),(0),(1),(1);
ANALYZE qp_outer;
ANALYZE qp_inner;
