SELECT o.b, max(o.a) AS group_max,
       (SELECT count(DISTINCT i.b)
        FROM dsl_bag_pair_left AS i WHERE i.a = o.b) AS n
FROM dsl_bag_pair_right AS o
GROUP BY o.b
ORDER BY o.b NULLS LAST;
