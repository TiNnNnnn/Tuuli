WITH q AS (SELECT id,random() AS r FROM dsl_insub_outer)
SELECT bool_and(a.r=b.r) FROM q a JOIN q b USING(id);
