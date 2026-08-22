CREATE TABLE account
(
    acctno integer NOT NULL,
    type varchar(20) NOT NULL,
    balance integer NOT NULL
);

CREATE TABLE bonus
(
    ename varchar(20) NOT NULL,
    job varchar(10) NOT NULL,
    sal integer NOT NULL,
    comm integer NOT NULL
);

CREATE TABLE dept
(
    deptno integer PRIMARY KEY,
    name varchar(10) NOT NULL
);

CREATE TABLE emp
(
    empno integer PRIMARY KEY,
    ename varchar(20) NOT NULL,
    job varchar(10) NOT NULL,
    mgr integer,
    hiredate integer NOT NULL,
    sal integer NOT NULL,
    comm integer NOT NULL,
    deptno integer REFERENCES dept (deptno),
    slacker integer
);

CREATE TABLE t
(
    k0 varchar(20) NOT NULL,
    c1 varchar(20),
    f1_a0 integer NOT NULL,
    f2_a0 integer NOT NULL,
    f0_c0 integer NOT NULL,
    f1_c0 integer,
    f0_c1 integer NOT NULL,
    f1_c2 integer NOT NULL,
    f2_c3 integer NOT NULL
);

CREATE TABLE anon (c integer);
