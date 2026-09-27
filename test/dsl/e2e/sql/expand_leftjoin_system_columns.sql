SELECT l.k, r.k, l.ctid IS NOT NULL AS left_present,
       r.ctid IS NOT NULL AS right_present
FROM dsl_loj_outer AS l LEFT JOIN dsl_loj_inner AS r ON l.k = r.k
ORDER BY l.k;
