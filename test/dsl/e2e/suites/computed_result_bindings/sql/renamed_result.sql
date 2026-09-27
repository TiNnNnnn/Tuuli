SELECT p.x AS y FROM (
    SELECT i + 10 AS x FROM computed_result_input
) AS p ORDER BY y NULLS FIRST;
