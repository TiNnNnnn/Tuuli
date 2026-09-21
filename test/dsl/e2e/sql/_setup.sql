CREATE EXTENSION IF NOT EXISTS pg_orca;

CREATE TABLE dsl_insub_outer(id int PRIMARY KEY, v int NOT NULL);
CREATE TABLE dsl_insub_inner(id int PRIMARY KEY);
INSERT INTO dsl_insub_outer VALUES (1,10),(2,20),(3,30);
INSERT INTO dsl_insub_inner VALUES (1),(3);

CREATE SEQUENCE dsl_compute_sequence;

CREATE FUNCTION dsl_next_token(int, int) RETURNS bigint
LANGUAGE plpgsql VOLATILE STRICT AS $$
BEGIN
    RETURN nextval('dsl_compute_sequence');
END
$$;
CREATE OPERATOR <#> (
    LEFTARG = int, RIGHTARG = int, FUNCTION = dsl_next_token
);

CREATE FUNCTION dsl_stable_token(int, int) RETURNS int
LANGUAGE plpgsql STABLE STRICT AS $$
BEGIN
    RETURN pg_backend_pid();
END
$$;
CREATE OPERATOR <##> (
    LEFTARG = int, RIGHTARG = int, FUNCTION = dsl_stable_token
);

CREATE FUNCTION dsl_next_flag(int, int) RETURNS boolean
LANGUAGE plpgsql VOLATILE STRICT AS $$
BEGIN
    RETURN nextval('dsl_compute_sequence') % 2 = 0;
END
$$;
CREATE OPERATOR #=# (
    LEFTARG = int, RIGHTARG = int, FUNCTION = dsl_next_flag
);

CREATE FUNCTION dsl_cast_token(int) RETURNS uuid
LANGUAGE plpgsql VOLATILE STRICT AS $$
BEGIN
    RETURN lpad(nextval('dsl_compute_sequence')::text, 32, '0')::uuid;
END
$$;
CREATE CAST (int AS uuid) WITH FUNCTION dsl_cast_token(int) AS IMPLICIT;

CREATE FUNCTION dsl_cast_stable_token(bigint) RETURNS uuid
LANGUAGE plpgsql STABLE STRICT AS $$
BEGIN
    RETURN lpad(pg_backend_pid()::text, 32, '0')::uuid;
END
$$;
CREATE CAST (bigint AS uuid) WITH FUNCTION dsl_cast_stable_token(bigint) AS IMPLICIT;

CREATE FUNCTION dsl_cast_null_flag(bigint) RETURNS boolean
LANGUAGE plpgsql IMMUTABLE CALLED ON NULL INPUT AS $$
BEGIN
    RETURN $1 IS NULL;
END
$$;
CREATE CAST (bigint AS boolean) WITH FUNCTION dsl_cast_null_flag(bigint) AS IMPLICIT;

CREATE FUNCTION dsl_cast_strict_flag(smallint) RETURNS boolean
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
BEGIN
    RETURN $1 = 1;
END
$$;
CREATE CAST (smallint AS boolean) WITH FUNCTION dsl_cast_strict_flag(smallint) AS IMPLICIT;

CREATE TABLE dsl_cast_text(id int PRIMARY KEY, value text NOT NULL);
INSERT INTO dsl_cast_text VALUES (1,'one'),(3,'three');

CREATE TABLE dsl_cast_safety(id int PRIMARY KEY, s smallint, v bigint, value text);
INSERT INTO dsl_cast_safety VALUES
    (1,-32768,9223372036854775807,'not an integer'),
    (2,32767,42,'42'),
    (3,NULL,NULL,NULL);

-- Real type I/O metadata, without changing any built-in function or cast.
-- The implementations are pure; volatility declarations test whether the
-- optimizer respects the catalog contract, not an observed side effect.
DO $$
DECLARE spec record;
BEGIN
    FOR spec IN SELECT * FROM (VALUES
        ('dsl_io_input','VOLATILE','IMMUTABLE'),
        ('dsl_io_output','IMMUTABLE','VOLATILE'),
        ('dsl_io_stable','STABLE','STABLE')
    ) AS types(name, input_stability, output_stability)
    LOOP
        EXECUTE format('CREATE TYPE %I', spec.name);
        EXECUTE format('CREATE FUNCTION %I(cstring) RETURNS %I LANGUAGE internal %s STRICT AS %L',
            spec.name || '_in', spec.name, spec.input_stability, 'int4in');
        EXECUTE format('CREATE FUNCTION %I(%I) RETURNS cstring LANGUAGE internal %s STRICT AS %L',
            spec.name || '_out', spec.name, spec.output_stability, 'int4out');
        EXECUTE format('CREATE TYPE %I (INPUT=%I, OUTPUT=%I, INTERNALLENGTH=4, PASSEDBYVALUE, ALIGNMENT=int4)',
            spec.name, spec.name || '_in', spec.name || '_out');
    END LOOP;
END
$$;
CREATE TABLE dsl_io_values(source_text text, source_output dsl_io_output, source_stable dsl_io_stable);
INSERT INTO dsl_io_values VALUES ('1','1','1'),('2','2','2'),('3','3','3');

CREATE TABLE dsl_correlated_exists(k int, payload int NOT NULL);
INSERT INTO dsl_correlated_exists VALUES (1,10),(NULL,20),(2,30);

CREATE TABLE dsl_agg_outer(g int, v int);
CREATE TABLE dsl_exists_inner(x int);
INSERT INTO dsl_agg_outer VALUES (1,10),(1,20),(2,NULL),(3,5);
INSERT INTO dsl_exists_inner VALUES (5),(20);

CREATE TABLE dsl_compute_group(label varchar(10) NOT NULL);
INSERT INTO dsl_compute_group VALUES ('a'),('delta'),('delta');

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

CREATE TABLE dsl_nullable_unique(a int UNIQUE);
INSERT INTO dsl_nullable_unique VALUES (NULL),(NULL),(1),(2);
CREATE TABLE dsl_nullable_composite_unique(a int NOT NULL, b int, UNIQUE(a,b));
INSERT INTO dsl_nullable_composite_unique VALUES (1,NULL),(1,NULL),(1,2),(2,3);
CREATE TABLE dsl_nulls_not_distinct_unique(a int UNIQUE NULLS NOT DISTINCT);
INSERT INTO dsl_nulls_not_distinct_unique VALUES (NULL),(1),(2);

CREATE TABLE dsl_loj_outer(k int NOT NULL);
CREATE TABLE dsl_loj_inner(k int NOT NULL);
INSERT INTO dsl_loj_outer VALUES (1),(3000);
INSERT INTO dsl_loj_inner SELECT generate_series(1, 2000);

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

CREATE TABLE dsl_residual_outer(id int PRIMARY KEY, status int);
CREATE TABLE dsl_residual_inner(id int, tag int, PRIMARY KEY(id,tag));
INSERT INTO dsl_residual_outer SELECT i,i%2 FROM generate_series(1,1024) AS i;
INSERT INTO dsl_residual_inner
SELECT i,tag FROM generate_series(1,1024) AS i
CROSS JOIN generate_series(1,4) AS tag WHERE i%5<>0;

CREATE TABLE dsl_fractional_nullable_ndv(k int);
INSERT INTO dsl_fractional_nullable_ndv VALUES
    (NULL),(NULL),(0),(1),(2),(3),(4),(5);

ANALYZE;

DO $$ BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_stats
                   WHERE tablename='dsl_fractional_nullable_ndv' AND attname='k'
                     AND n_distinct=-0.75 AND null_frac=0.25) THEN
        RAISE EXCEPTION 'fractional nullable NDV fixture statistics differ';
    END IF;
END $$;
