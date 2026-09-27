SELECT DISTINCT q.x FROM (
    SELECT DISTINCT 10 / i AS x, label FROM distinct_input
) AS q;
