SELECT empno, count(*), count(DISTINCT deptno)
FROM dsl_dqa
GROUP BY empno
ORDER BY empno;

