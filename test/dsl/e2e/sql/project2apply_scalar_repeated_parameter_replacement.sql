SELECT o.empno, o.deptno,
       (SELECT (SELECT dsl_statement_flag(i.id, o.deptno)
                FROM dsl_insub_inner AS i WHERE i.id = 1)
        FROM dsl_insub_outer AS anchor WHERE anchor.v = o.deptno LIMIT 1) AS scalar_value,
       current_setting('dsl_test.flag_calls', true)::int AS calls
FROM dsl_dqa AS o
ORDER BY o.empno;
