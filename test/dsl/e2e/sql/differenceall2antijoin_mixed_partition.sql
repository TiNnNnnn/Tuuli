SELECT a::bigint AS k, b < 2 AS flag FROM dsl_bag_pair_left
EXCEPT ALL
SELECT a::bigint, b < 2 FROM dsl_bag_pair_right
ORDER BY k NULLS LAST, flag NULLS LAST;
