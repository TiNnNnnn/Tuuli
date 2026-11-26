SELECT o.b, (SELECT count(DISTINCT i.b)
             FROM dsl_bag_pair_left AS i WHERE i.a = o.b HAVING count(DISTINCT i.b) > 0::bigint) AS n
FROM dsl_bag_pair_right AS o
ORDER BY o.b NULLS LAST;
