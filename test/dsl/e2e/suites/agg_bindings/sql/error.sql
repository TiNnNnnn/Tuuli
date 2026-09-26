SELECT max(a / (g-g)) FROM agg_bind WHERE g > 0 HAVING max(a) < 0;
