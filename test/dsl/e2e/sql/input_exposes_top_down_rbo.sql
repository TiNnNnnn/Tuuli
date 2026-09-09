SELECT q.empno
FROM (
    SELECT DISTINCT empno
    FROM dsl_dqa
) AS q
GROUP BY q.empno
ORDER BY q.empno;
