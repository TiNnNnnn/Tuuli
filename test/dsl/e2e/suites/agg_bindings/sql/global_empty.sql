SELECT count(*), COALESCE(sum(a),-1) FROM agg_bind WHERE a > 99 HAVING count(*) = 0;
