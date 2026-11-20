SELECT b < 3 AS flag, row_number() OVER (PARTITION BY a ORDER BY b) AS pos
FROM dsl_bag_pair_left
INTERSECT ALL
SELECT b < 3, row_number() OVER (PARTITION BY a ORDER BY b)
FROM dsl_bag_pair_right
ORDER BY flag NULLS LAST, pos;
