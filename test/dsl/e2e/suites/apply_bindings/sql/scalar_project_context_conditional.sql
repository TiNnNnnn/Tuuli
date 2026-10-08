SELECT CASE WHEN i IS NULL
            THEN (SELECT v FROM (VALUES (2,true)) AS scalar_input(id,v) WHERE id = 2)
            ELSE false END AS n,
       i::bigint AS x
FROM binding_left ORDER BY x NULLS FIRST;
