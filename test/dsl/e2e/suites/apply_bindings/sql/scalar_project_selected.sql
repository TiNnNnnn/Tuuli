SELECT (SELECT v FROM (VALUES (2,true)) AS scalar_input(id,v)
        WHERE id = 2) AS v
FROM binding_left ORDER BY v;
