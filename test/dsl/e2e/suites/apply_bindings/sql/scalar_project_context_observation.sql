SELECT (SELECT v FROM (VALUES (2,true)) AS scalar_input(id,v)
        WHERE id = 2) IS NULL AS n, i::bigint AS x
FROM binding_left ORDER BY x NULLS FIRST;
