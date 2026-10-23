SELECT (SELECT v.x FROM (VALUES (1), (2), (3)) AS v(x)) AS scalar_value,
       1 / (o.id - o.id) AS after_subquery
FROM dsl_insub_outer AS o LIMIT 1;
