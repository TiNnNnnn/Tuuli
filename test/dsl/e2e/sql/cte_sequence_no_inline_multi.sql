WITH q AS (SELECT k FROM dsl_eq_left) SELECT a.k,count(*) FROM q a JOIN q b USING(k) GROUP BY a.k ORDER BY a.k;
