SELECT i FROM qp_outer
WHERE v = ANY (SELECT coalesce(v,2) FROM qp_inner)
ORDER BY i;
