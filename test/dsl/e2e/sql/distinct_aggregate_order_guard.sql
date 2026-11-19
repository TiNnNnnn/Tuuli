SELECT deptno, array_agg(DISTINCT empno)
FROM dsl_dqa
GROUP BY deptno
ORDER BY deptno;
