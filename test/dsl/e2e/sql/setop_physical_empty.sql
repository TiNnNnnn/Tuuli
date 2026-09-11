SELECT 'intersect', count(*) FROM (
 SELECT a,b FROM dsl_bag_pair_left WHERE a < 0
 INTERSECT SELECT a,b FROM dsl_bag_pair_right
) s
UNION ALL
SELECT 'intersect_all', count(*) FROM (
 SELECT a,b FROM dsl_bag_pair_left
 INTERSECT ALL SELECT a,b FROM dsl_bag_pair_right WHERE a < 0
) s
UNION ALL
SELECT 'except', count(*) FROM (
 SELECT a,b FROM dsl_bag_pair_left
 EXCEPT SELECT a,b FROM dsl_bag_pair_right WHERE a < 0
) s
UNION ALL
SELECT 'except_all', count(*) FROM (
 SELECT a,b FROM dsl_bag_pair_left WHERE a < 0
 EXCEPT ALL SELECT a,b FROM dsl_bag_pair_right
) s ORDER BY 1;
