SELECT q.y, q.x FROM (
    SELECT p.x, p.y FROM (
        SELECT i + 10 AS x, j + 100 AS y FROM projection_input
    ) AS p
) AS q ORDER BY q.y NULLS FIRST, q.x NULLS FIRST;
