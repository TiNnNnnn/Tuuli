SELECT i::bigint AS x, (SELECT v FROM (VALUES (2,true)) AS s(id,v)
                       WHERE id = 2) IS NULL AS n, i IS NULL AS y
FROM binding_left ORDER BY x NULLS FIRST;
