SELECT sum((SELECT max(i.payload)
            FROM dsl_correlated_exists AS i)) AS total_payload
FROM dsl_insub_outer;
