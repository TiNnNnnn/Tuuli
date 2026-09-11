WITH q AS (SELECT set_id,y,y+10 AS v FROM dsl_notin_inner) SELECT a.v,b.y FROM q a JOIN q b ON a.set_id=b.set_id WHERE a.set_id=3 ORDER BY a.v NULLS FIRST,b.y NULLS FIRST;
