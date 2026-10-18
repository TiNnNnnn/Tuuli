SELECT DISTINCT a.empno
FROM dsl_dqa AS a
JOIN dsl_insub_outer AS b
  ON a.empno = b.id AND a.deptno < b.v
ORDER BY a.empno;
