SELECT o.g, (SELECT max(o.v) FROM dsl_agg_outer AS i
             WHERE i.g = o.g AND i.v < 10) AS outer_max
FROM dsl_agg_outer AS o
GROUP BY o.g
ORDER BY o.g;
