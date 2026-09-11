-- The empty tuple is one equivalence class, with ordinary bag multiplicity.
SELECT 'intersect', count(*) FROM (
 SELECT FROM dsl_bag_pair_left INTERSECT SELECT FROM dsl_bag_pair_right
) s
UNION ALL
SELECT 'intersect_all', count(*) FROM (
 SELECT FROM dsl_bag_pair_left INTERSECT ALL SELECT FROM dsl_bag_pair_right
) s
UNION ALL
SELECT 'except', count(*) FROM (
 SELECT FROM dsl_bag_pair_left EXCEPT SELECT FROM dsl_bag_pair_right
) s
UNION ALL
SELECT 'except_all', count(*) FROM (
 SELECT FROM dsl_bag_pair_left EXCEPT ALL SELECT FROM dsl_bag_pair_right
) s ORDER BY 1;
