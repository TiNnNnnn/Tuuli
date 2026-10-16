SELECT DISTINCT a.empno
FROM dsl_dqa a JOIN dsl_insub_outer b ON a.empno = b.id
WHERE b.v > 10
ORDER BY a.empno;
