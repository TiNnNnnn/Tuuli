SELECT 'any-empty', i FROM qp_outer
WHERE v = ANY (SELECT v FROM (SELECT v, generate_series(1,0) FROM qp_inner) AS s)
UNION ALL
SELECT 'all-empty', i FROM qp_outer
WHERE v = ALL (SELECT v FROM (SELECT v, generate_series(1,0) FROM qp_inner) AS s)
UNION ALL
SELECT 'any-live', i FROM qp_outer
WHERE v = ANY (SELECT v FROM (SELECT v, generate_series(1,v) FROM qp_inner) AS s)
UNION ALL
SELECT 'all-live', i FROM qp_outer
WHERE v = ALL (SELECT v FROM (SELECT v, generate_series(1,v) FROM qp_inner) AS s)
ORDER BY 1, 2;
