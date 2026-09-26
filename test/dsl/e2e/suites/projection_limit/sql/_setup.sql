CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE dept(deptno integer PRIMARY KEY, name varchar(10) NOT NULL);
CREATE TABLE emp(empno integer PRIMARY KEY, deptno integer NOT NULL REFERENCES dept);
INSERT INTO dept VALUES (20,'eng');
INSERT INTO emp VALUES (1,20);
ANALYZE dept;
ANALYZE emp;
