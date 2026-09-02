SELECT sum(CASE WHEN EXISTS (
                     SELECT 1
                     FROM dsl_correlated_exists AS e
                     WHERE e.k = o.id)
                THEN (SELECT max(s.payload)
                      FROM dsl_correlated_exists AS s
                      WHERE s.k = o.id)
                ELSE 0 END) AS matched_payload_sum
FROM dsl_insub_outer AS o;
