SELECT COALESCE(g,-1), max(a), sum(a), count(a)
FROM agg_bind GROUP BY g HAVING max(a) > 10 ORDER BY 1;
