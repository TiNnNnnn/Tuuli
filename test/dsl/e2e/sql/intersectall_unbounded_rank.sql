SELECT l.a FROM dsl_bag_pair_left AS l CROSS JOIN dsl_bag_pair_right AS r
INTERSECT ALL VALUES (1), (1)
ORDER BY 1;
