-- Rebinding must rebuild counts for the new outer parameter.
SELECT o.k, count(*) FROM (VALUES (1),(2),(1)) o(k)
CROSS JOIN LATERAL (
 SELECT a,b FROM dsl_bag_pair_left WHERE a = o.k
 INTERSECT ALL
 SELECT a,b FROM dsl_bag_pair_right WHERE a = o.k
 OFFSET 0
) s GROUP BY o.k ORDER BY 1;
