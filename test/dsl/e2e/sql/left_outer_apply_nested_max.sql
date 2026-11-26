SELECT ancestor.payload,
       (SELECT sum(coalesce((SELECT max(leaf.payload)
                             FROM dsl_correlated_exists AS leaf
                             WHERE leaf.k = local_row.id), -1))
        FROM dsl_insub_outer AS local_row
        WHERE local_row.id >= ancestor.id)
FROM (VALUES (1,900), (3,901), (4,902)) AS ancestor(id,payload)
ORDER BY ancestor.payload;
