SELECT DISTINCT q.i, q.label FROM (
    SELECT DISTINCT i, label, 10 / extra AS discarded FROM distinct_multi
) AS q ORDER BY q.i NULLS LAST, q.label NULLS LAST;
