SELECT o.id, (SELECT i.payload FROM dsl_correlated_exists i) AS subquery_value
FROM dsl_insub_outer o ORDER BY o.id;
