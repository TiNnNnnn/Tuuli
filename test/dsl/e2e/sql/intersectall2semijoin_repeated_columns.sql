SELECT a,a,b FROM dsl_bag_pair_left
INTERSECT ALL
SELECT a,b,b FROM dsl_bag_pair_right
ORDER BY 1,2,3;
