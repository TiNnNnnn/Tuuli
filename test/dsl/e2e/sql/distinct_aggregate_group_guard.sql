SELECT deptno, count(*), count(DISTINCT empno % 1)
FROM dsl_dqa
GROUP BY deptno
ORDER BY deptno;
