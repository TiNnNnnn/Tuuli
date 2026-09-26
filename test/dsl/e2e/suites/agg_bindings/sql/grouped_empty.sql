SELECT g, max(a) FROM agg_bind WHERE a > 99 GROUP BY g HAVING max(a) > 10;
