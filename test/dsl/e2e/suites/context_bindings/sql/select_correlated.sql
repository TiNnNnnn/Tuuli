SELECT o.i, o.j,
       (SELECT (q.j > o.i) IS NOT TRUE
        FROM context_input q WHERE q.i = 1 LIMIT 1) AS value
FROM context_input o
ORDER BY o.i NULLS FIRST, o.j;
