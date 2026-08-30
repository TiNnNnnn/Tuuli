SELECT empno, deptno, max(deptno)
FROM dsl_dqa
GROUP BY empno, deptno
ORDER BY empno;
