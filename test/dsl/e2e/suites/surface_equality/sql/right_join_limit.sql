-- Original Calcite 201 shape, nonempty results and stable legacy rule identities.
SELECT * FROM emp
RIGHT JOIN (SELECT * FROM dept LIMIT 10) AS t ON emp.deptno = t.deptno
LIMIT 10;
