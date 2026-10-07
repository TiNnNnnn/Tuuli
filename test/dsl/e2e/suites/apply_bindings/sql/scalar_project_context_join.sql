SELECT ((SELECT min(b.i) FROM binding_right b
         JOIN binding_left c ON b.i = c.i
         WHERE c.i = a.i) + a.i) * 2 AS s,
       a.i + 1 AS x, a.i * 2 AS y
FROM binding_left a ORDER BY x NULLS FIRST;
