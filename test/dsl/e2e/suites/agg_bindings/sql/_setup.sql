CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE agg_bind(g int, a int);
INSERT INTO agg_bind VALUES (1,10),(1,10),(1,NULL),(2,20),(2,-5),(NULL,30);
ANALYZE agg_bind;
