SELECT DISTINCT q.i FROM (
    SELECT DISTINCT i, label FROM distinct_empty
) AS q ORDER BY q.i NULLS LAST;
