WITH q AS (SELECT k FROM dsl_eq_left WHERE k>1000) SELECT count(*) FROM q;
