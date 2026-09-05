CREATE EXTENSION pg_orca;

CREATE TABLE dsl_insub_outer(id int PRIMARY KEY, v int NOT NULL);
CREATE TABLE dsl_insub_inner(id int PRIMARY KEY);
INSERT INTO dsl_insub_outer VALUES (1,10),(2,20),(3,30);
INSERT INTO dsl_insub_inner VALUES (1),(3);

CREATE TABLE dsl_correlated_exists(k int, payload int NOT NULL);
INSERT INTO dsl_correlated_exists VALUES (1,10),(NULL,20),(2,30);

CREATE TABLE dsl_agg_outer(g int, v int);
CREATE TABLE dsl_exists_inner(x int);
INSERT INTO dsl_agg_outer VALUES (1,10),(1,20),(2,NULL),(3,5);
INSERT INTO dsl_exists_inner VALUES (5),(20);

CREATE TABLE dsl_dqa(empno int PRIMARY KEY, deptno int NOT NULL);
INSERT INTO dsl_dqa VALUES (1,10),(2,10),(3,20);

CREATE TABLE dsl_fk_parent(id int PRIMARY KEY, payload int NOT NULL);
CREATE TABLE dsl_notin_tag(id int PRIMARY KEY);
CREATE TABLE dsl_fk_child(
    id int PRIMARY KEY,
    parent_id int NOT NULL REFERENCES dsl_fk_parent(id));
CREATE TABLE dsl_fk_nullable_child(
    id int PRIMARY KEY,
    parent_id int REFERENCES dsl_fk_parent(id));
INSERT INTO dsl_fk_parent VALUES (10,100),(20,200);
INSERT INTO dsl_notin_tag VALUES (1),(2),(3);
INSERT INTO dsl_fk_child VALUES (1,10),(2,10),(3,20);
INSERT INTO dsl_fk_nullable_child VALUES (1,10),(2,NULL);

CREATE TABLE dsl_composite_fk_parent(
    k1 int,
    k2 int,
    payload int NOT NULL,
    PRIMARY KEY (k1, k2));
CREATE TABLE dsl_composite_fk_child(
    id int PRIMARY KEY,
    k1 int,
    k2 int,
    FOREIGN KEY (k1, k2) REFERENCES dsl_composite_fk_parent(k1, k2));
INSERT INTO dsl_composite_fk_parent VALUES (10,100,1),(20,200,2);
INSERT INTO dsl_composite_fk_child VALUES
    (1,10,100),(2,10,100),(3,20,200),(4,NULL,NULL);

CREATE TABLE dsl_eq_left(k int NOT NULL);
CREATE TABLE dsl_eq_right(k int NOT NULL);
INSERT INTO dsl_eq_left VALUES (1),(1),(2),(3);
INSERT INTO dsl_eq_right VALUES (1),(2),(2),(4);

CREATE TABLE dsl_eq_pair_left(a int, b int);
CREATE TABLE dsl_eq_pair_right(a int, b int);
INSERT INTO dsl_eq_pair_left VALUES
    (1,1),(1,1),(1,2),(2,2),(2,NULL),(NULL,3);
INSERT INTO dsl_eq_pair_right VALUES
    (1,1),(1,1),(2,2),(2,NULL),(NULL,3);

CREATE TABLE dsl_bag_pair_left(a int, b int);
CREATE TABLE dsl_bag_pair_right(a int, b int);
INSERT INTO dsl_bag_pair_left VALUES
    (1,1),(1,1),(1,1),(1,2),(2,2),(2,2),(2,NULL),(NULL,3);
INSERT INTO dsl_bag_pair_right VALUES
    (1,1),(1,1),(2,2),(2,2),(2,2),(2,NULL),(NULL,3);

CREATE TABLE dsl_notin_outer(case_id int PRIMARY KEY, set_id int, x int);
CREATE TABLE dsl_notin_inner(set_id int, y int);
INSERT INTO dsl_notin_outer VALUES
    (1,1,NULL),
    (2,1,1),
    (3,2,1),
    (4,3,1),
    (5,4,1),
    (6,4,NULL);
INSERT INTO dsl_notin_inner VALUES
    (2,1),(2,2),
    (3,2),(3,NULL),
    (4,2),(4,3);

ANALYZE;
