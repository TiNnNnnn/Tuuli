SELECT ((SELECT min(c.i) FROM binding_right b
         LEFT JOIN binding_left c ON b.i = c.i
         WHERE b.i > a.i) + a.i) * 2 AS s,
       a.i + 1 AS x, a.i * 2 AS y
FROM binding_left a ORDER BY x NULLS FIRST;
