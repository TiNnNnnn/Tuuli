WITH a AS (SELECT k FROM dsl_eq_left),b AS (SELECT k,k+10 AS v FROM a) SELECT v FROM b ORDER BY v;
