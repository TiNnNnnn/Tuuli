SELECT o.id,
       (o.id > 0) = ANY (SELECT ranked.max_payload > 0
        FROM (SELECT max(i.payload) OVER
                     (PARTITION BY i.k ORDER BY i.seq
                      ROWS BETWEEN '1'::bigint PRECEDING AND CURRENT ROW) AS max_payload
              FROM dsl_window_partitions AS i WHERE i.k = o.k) AS ranked
        WHERE ranked.max_payload IS NOT NULL)
FROM dsl_window_partitions AS o ORDER BY o.id;
