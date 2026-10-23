SELECT 1 / (o.id - o.id) AS before_subquery,
       (SELECT v.x FROM (VALUES (1), (2), (3)) AS v(x)) AS scalar_value
FROM dsl_insub_outer AS o LIMIT 1;
