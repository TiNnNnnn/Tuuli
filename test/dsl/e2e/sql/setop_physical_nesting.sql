-- Keep all comparison keys even though the parent projects only a.
SELECT 'left_associative', COALESCE(a,-1) FROM (
 SELECT a,b FROM dsl_bag_pair_left
 EXCEPT ALL SELECT a,b FROM dsl_bag_pair_right
 EXCEPT ALL SELECT a,b FROM dsl_bag_pair_left WHERE b = 2
) s
UNION ALL
SELECT 'right_nested', COALESCE(a,-1) FROM (
 SELECT a,b FROM dsl_bag_pair_left
 EXCEPT ALL (
   SELECT a,b FROM dsl_bag_pair_right
   EXCEPT ALL SELECT a,b FROM dsl_bag_pair_left
 )
) s
UNION ALL
SELECT 'intersect_nary', COALESCE(a,-1) FROM (
 SELECT a,b FROM dsl_bag_pair_left
 INTERSECT ALL SELECT a,b FROM dsl_bag_pair_right
 INTERSECT ALL SELECT a,b FROM dsl_bag_pair_left WHERE a = 1
) s ORDER BY 1,2;
