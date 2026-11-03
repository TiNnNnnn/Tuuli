SELECT q.id, NOT q.inner_value AS dependent_value, q.inner_value IS FALSE AS second_dependent_value
FROM (SELECT id, v, v > 5 AS inner_value FROM dsl_insub_outer) AS q
ORDER BY q.id;
