SELECT DISTINCT q.i FROM (
    SELECT DISTINCT i, label FROM distinct_input
) AS q ORDER BY q.i NULLS LAST;
