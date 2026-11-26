SELECT o.b, (SELECT count(DISTINCT i.b)
             FROM dsl_bag_pair_left AS i WHERE i.a = o.b) AS n
FROM dsl_bag_pair_right AS o
ORDER BY o.b NULLS LAST;
