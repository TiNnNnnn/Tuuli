-- Physical demand probes, not rule-equivalence certificates. A correlated
-- right input must retain its row prefix before a later scalar error.
-- Pin the actual forward index path below; do not infer order from unordered
-- SQL or mistake a lazy Materialize node for a full blocking Sort.
BEGIN;
LOAD 'pg_orca';
SET LOCAL statement_timeout = '1min';
SET LOCAL max_parallel_workers_per_gather = 0;
SET LOCAL jit = off;
SET LOCAL enable_hashjoin = off;
SET LOCAL enable_mergejoin = off;
SET LOCAL enable_seqscan = off;
SET LOCAL enable_bitmapscan = off;
SET LOCAL pg_orca.enable_dsl_rule = off;
SET LOCAL pg_orca.trace_fallback = on;
CREATE TEMP TABLE demand_loop_outer(k integer PRIMARY KEY);
CREATE TEMP TABLE demand_loop_inner(owner integer, k integer, divisor integer, PRIMARY KEY(owner,k));
INSERT INTO demand_loop_outer VALUES (1), (2);
INSERT INTO demand_loop_inner VALUES (1,1,1), (1,2,0), (2,1,0);
ANALYZE demand_loop_outer;
ANALYZE demand_loop_inner;

DO $probe$
DECLARE
  engine text;
  outer_key integer;
  bound integer;
  query text;
  plan_line text;
  plan_text text;
  result_row record;
  result_count integer;
  result_state text;
  expected_state text;
BEGIN
  FOREACH engine IN ARRAY ARRAY['postgres', 'orca'] LOOP
    PERFORM set_config('pg_orca.enable_orca', (engine = 'orca')::text, true);
    FOR outer_key IN 1..3 LOOP
      FOR bound IN 0..2 LOOP
        query := format('SELECT r.value FROM demand_loop_outer l CROSS JOIN LATERAL '
          || '(SELECT 10 / divisor AS value FROM demand_loop_inner r '
          || 'WHERE r.owner = l.k AND r.k >= 1 OFFSET 0) r WHERE l.k = %s LIMIT %s', outer_key, bound);
        plan_text := '';
        FOR plan_line IN EXECUTE 'EXPLAIN (VERBOSE, COSTS ON) ' || query LOOP
          plan_text := plan_text || plan_line || E'\n';
        END LOOP;
        IF (engine = 'orca') <> (position('Optimizer: pg_orca' IN plan_text) > 0)
            OR (bound > 0 AND outer_key < 3 AND
                (position('Nested Loop' IN plan_text) = 0 OR position('Sort' IN plan_text) > 0
                 OR position('using demand_loop_inner_pkey' IN plan_text) = 0
                 OR position('Backward' IN plan_text) > 0)) THEN
          RAISE EXCEPTION 'unexpected inner-loop demand plan: %', plan_text;
        END IF;
        result_count := 0;
        result_state := '00000';
        BEGIN
          FOR result_row IN EXECUTE query LOOP
            IF result_row.value IS DISTINCT FROM 10 THEN
              RAISE EXCEPTION 'unexpected inner-loop row: %', result_row;
            END IF;
            result_count := result_count + 1;
          END LOOP;
        EXCEPTION WHEN division_by_zero THEN
          result_state := SQLSTATE;
          result_count := NULL;
        END;
        expected_state := CASE WHEN bound = 0 OR outer_key = 3 OR (outer_key = 1 AND bound = 1)
          THEN '00000' ELSE '22012' END;
        IF result_state <> expected_state OR
            (result_state = '00000' AND result_count <> CASE WHEN bound = 0 OR outer_key = 3 THEN 0 ELSE 1 END) THEN
          RAISE EXCEPTION 'unexpected inner-loop demand: %, %, %, %, %', engine, outer_key, bound, result_state, result_count;
        END IF;
        RAISE NOTICE '%', json_build_object('engine', engine, 'outer_key', outer_key,
          'bound', bound, 'sqlstate', result_state, 'rows', result_count, 'plan', plan_text);
      END LOOP;
    END LOOP;
  END LOOP;
END
$probe$;
ROLLBACK;
