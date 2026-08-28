SELECT d.empno, o.id
FROM dsl_dqa AS d
INNER JOIN dsl_insub_outer AS o ON d.empno = o.id
GROUP BY d.empno, o.id
ORDER BY d.empno, o.id;
