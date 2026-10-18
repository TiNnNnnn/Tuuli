SELECT DISTINCT a.empno
FROM (SELECT empno, deptno + 1 AS next_dept FROM dsl_dqa) AS a
JOIN (SELECT id, v + 1 AS next_v FROM dsl_insub_outer) AS b
  ON a.empno = b.id AND a.next_dept < b.next_v
ORDER BY a.empno;
