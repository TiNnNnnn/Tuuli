-- Calcite 201's original shape. One nonempty row avoids arbitrary LIMIT ties.
SELECT * FROM emp
RIGHT JOIN (SELECT * FROM dept LIMIT 10) AS t ON emp.deptno = t.deptno
LIMIT 10;
