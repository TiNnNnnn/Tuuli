SELECT 'any-live', i FROM qp_outer
WHERE v < ANY (SELECT v FROM qp_inner) AND i >= 2
UNION ALL
SELECT 'all-live', i FROM qp_outer
WHERE i >= 2 AND v > ALL (SELECT v FROM qp_inner WHERE v IS NOT NULL)
UNION ALL
SELECT 'all-empty', i FROM qp_outer
WHERE v < ALL (SELECT v FROM qp_inner WHERE v < 0) AND i <= 3
ORDER BY 1, 2;
