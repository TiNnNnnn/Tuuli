SELECT DISTINCT q.n FROM (
    SELECT i, count(*) AS n FROM distinct_input GROUP BY i
) AS q ORDER BY q.n;
