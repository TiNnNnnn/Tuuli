WITH q AS (SELECT k FROM dsl_eq_left)
SELECT o.id,q.k FROM dsl_insub_outer o CROSS JOIN q ORDER BY o.id,q.k;
