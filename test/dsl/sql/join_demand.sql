-- Run with psql -X -v ON_ERROR_STOP=1 against an isolated PG/ORCA instance.
-- This probes executor demand, not DSL equivalence or replacement coverage.
-- No ANALYZE on demand_left: its filtered-empty sorted path must have a higher
-- startup cost than building the one-row hash table. Pin the resulting PG plan
-- below so a planner change cannot silently invalidate this witness.
-- The two nonempty controls distinguish skipped errors from swallowed errors.
BEGIN;
LOAD 'pg_orca';
CREATE TEMP TABLE demand_left(k integer);
CREATE TEMP TABLE demand_right(k integer, divisor integer);
INSERT INTO demand_left SELECT generate_series(1, 1000);
INSERT INTO demand_right VALUES (-1, 0);
ANALYZE demand_right;
SET LOCAL max_parallel_workers_per_gather = 0;
SET LOCAL enable_mergejoin = off;
SET LOCAL jit = off;
SET LOCAL client_min_messages = notice;
SET LOCAL statement_timeout = '1min';
SET LOCAL pg_orca.enable_dsl_rule = off;

DO $probe$
DECLARE
  engine text;
  algorithm text;
  variant text;
  scenario text;
  query text;
  plan_line text;
  plan_text text;
  result_count bigint;
  result_row record;
  result_state text;
BEGIN
  FOREACH scenario IN ARRAY ARRAY['empty-error', 'row-error', 'row-safe'] LOOP
    IF scenario = 'row-error' THEN
      INSERT INTO demand_left VALUES (-1);
    ELSIF scenario = 'row-safe' THEN
      UPDATE demand_right SET divisor = 1;
    END IF;
    FOREACH engine IN ARRAY ARRAY['postgres', 'orca'] LOOP
      PERFORM set_config('pg_orca.enable_orca', (engine = 'orca')::text, true);
      FOREACH algorithm IN ARRAY ARRAY['nested', 'hash'] LOOP
        PERFORM set_config('enable_nestloop', (algorithm = 'nested')::text, true);
        PERFORM set_config('enable_hashjoin', (algorithm = 'hash')::text, true);
        FOREACH variant IN ARRAY ARRAY['semi', 'left'] LOOP
          query := 'SELECT l.k FROM (SELECT k FROM demand_left WHERE k < 0 ORDER BY k OFFSET 0) l ';
          IF variant = 'semi' THEN
            query := query || 'WHERE EXISTS (SELECT FROM demand_right r WHERE r.k = l.k AND r.divisor <> l.k AND 10 / r.divisor > 0)';
          ELSE
            query := query || 'LEFT JOIN (SELECT k FROM demand_right WHERE 10 / divisor > 0 OFFSET 0) r ON l.k = r.k';
          END IF;
          plan_text := '';
          FOR plan_line IN EXECUTE 'EXPLAIN (COSTS ON) ' || query LOOP
            plan_text := plan_text || plan_line || E'\n';
          END LOOP;
          IF (engine = 'orca' AND position('Optimizer: pg_orca' IN plan_text) = 0)
              OR (engine = 'postgres' AND position('Optimizer: pg_orca' IN plan_text) > 0)
              OR (algorithm = 'nested' AND position('Nested Loop' IN plan_text) = 0)
              OR (algorithm = 'hash' AND plan_text !~ 'Hash[^\n]*Join') THEN
            RAISE EXCEPTION 'unexpected probe plan: %', plan_text;
          END IF;
          result_count := 0;
          result_state := '00000';
          BEGIN
            FOR result_row IN EXECUTE query LOOP
              IF result_row.k IS DISTINCT FROM -1 THEN
                RAISE EXCEPTION 'unexpected probe row: %', result_row;
              END IF;
              result_count := result_count + 1;
            END LOOP;
          EXCEPTION WHEN division_by_zero THEN
            result_state := SQLSTATE;
            result_count := NULL;
          END;
          IF (scenario = 'row-error' AND result_state <> '22012')
              OR (scenario = 'row-safe' AND (result_state <> '00000' OR result_count <> 1))
              OR (scenario = 'empty-error' AND variant = 'left'
                  AND (result_state <> '00000' OR result_count <> 0))
              OR (scenario = 'empty-error' AND result_state = '00000' AND result_count <> 0) THEN
            RAISE EXCEPTION 'unexpected demand: %, %, %, %, %, %',
              scenario, engine, algorithm, variant, result_state, result_count;
          END IF;
          -- Pin the two PG Semi plans before comparing their observed demand.
          -- ORCA may legitimately choose a different logical alternative; report
          -- its actual plan rather than assuming it kept a physical Semi Join.
          IF scenario = 'empty-error' AND engine = 'postgres' AND variant = 'semi'
              AND ((algorithm = 'nested' AND
                    (position('Nested Loop Semi Join' IN plan_text) = 0 OR result_state <> '00000'))
                OR (algorithm = 'hash' AND
                    (position('Hash Semi Join' IN plan_text) = 0 OR result_state <> '22012'))) THEN
            RAISE EXCEPTION 'PG demand witness changed: %, %', result_state, plan_text;
          END IF;
          RAISE NOTICE '%', json_build_object('engine', engine, 'algorithm', algorithm,
            'scenario', scenario, 'variant', variant, 'sqlstate', result_state,
            'rows', result_count, 'plan', plan_text);
        END LOOP;
      END LOOP;
    END LOOP;
  END LOOP;
END
$probe$;
ROLLBACK;
