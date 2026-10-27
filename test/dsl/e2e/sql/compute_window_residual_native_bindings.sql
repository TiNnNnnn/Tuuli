SELECT o.payload,
       (SELECT ranked.max_payload > 0
        FROM (SELECT max(i.payload) OVER (PARTITION BY i.k) AS max_payload
              FROM dsl_correlated_exists AS i WHERE i.k = o.k) AS ranked
        WHERE ranked.max_payload > 0)
FROM dsl_correlated_exists AS o ORDER BY o.payload;
