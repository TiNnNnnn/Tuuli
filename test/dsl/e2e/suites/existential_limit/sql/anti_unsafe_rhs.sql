SELECT o.k FROM dsl_eq_left AS o WHERE NOT EXISTS (SELECT i.k FROM dsl_eq_right AS i WHERE 100 / (i.k + 10) > 0 LIMIT '1'::bigint OFFSET '1'::bigint) ORDER BY o.k;
