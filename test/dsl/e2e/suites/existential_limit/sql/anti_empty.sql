SELECT o.k FROM dsl_eq_left AS o WHERE NOT EXISTS (SELECT i.k FROM dsl_eq_right AS i WHERE i.k > 1000 LIMIT '1'::bigint OFFSET '1'::bigint) ORDER BY o.k;
