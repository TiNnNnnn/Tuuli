SELECT l.x, l.y FROM (VALUES (1,10),(1,10),(NULL::integer,20),(2,30)) AS l(x,y)
UNION
SELECT r.x, r.y FROM (VALUES (10,1),(10,1),(20,NULL::integer),(40,3)) AS r(y,x)
ORDER BY 1 NULLS LAST, 2;
