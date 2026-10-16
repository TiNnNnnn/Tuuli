SELECT a.i + 1 AS x,
       (SELECT min(b.i) FROM binding_right b WHERE b.i = a.i) + 1 AS s,
       a.i * 2 AS y
FROM binding_left a ORDER BY x NULLS FIRST;
