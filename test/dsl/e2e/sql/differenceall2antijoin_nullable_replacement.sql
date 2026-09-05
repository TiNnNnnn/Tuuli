SELECT a, b FROM dsl_bag_pair_left
EXCEPT ALL
SELECT a, b FROM dsl_bag_pair_right
ORDER BY a NULLS LAST, b NULLS LAST;
