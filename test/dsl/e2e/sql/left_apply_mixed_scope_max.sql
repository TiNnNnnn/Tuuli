SELECT o.g, o.v, (SELECT max(i.v + o.v) FROM dsl_agg_outer AS i
                  WHERE i.g = o.g) AS mixed_max
FROM dsl_agg_outer AS o
ORDER BY o.g, o.v;
