SELECT 10 / (id - 1) AS v FROM dsl_insub_outer
UNION
SELECT v FROM dsl_insub_outer;
