-- Run with psql -X -v ON_ERROR_STOP=1 against an isolated PG/ORCA instance.
-- Demand probes only: no DSL rewrite or replacement coverage is inferred.
BEGIN;
LOAD 'pg_orca';
SET LOCAL statement_timeout = '1min';
SET LOCAL max_parallel_workers_per_gather = 0;
SET LOCAL jit = off;
SET LOCAL enable_seqscan = off;
SET LOCAL enable_bitmapscan = off;
SET LOCAL enable_indexscan = on;
SET LOCAL pg_orca.enable_dsl_rule = off;
SET LOCAL pg_orca.trace_fallback = on;
CREATE TEMP TABLE demand_subquery(k integer PRIMARY KEY, divisor integer);
INSERT INTO demand_subquery VALUES (1, 1), (2, 1), (3, 0);
ANALYZE demand_subquery;
CREATE TEMP TABLE demand_subquery_outer(k integer PRIMARY KEY);
INSERT INTO demand_subquery_outer VALUES (1);
ANALYZE demand_subquery_outer;

DO $probe$
DECLARE
  engine text;
  variant text;
  scenario text;
  condition text;
  query text;
  plan_line text;
  plan_text text;
  result_state text;
  result_value text;
  expected_state text;
  expected_value text;
  blocking boolean;
  division_present boolean;
BEGIN
  FOREACH engine IN ARRAY ARRAY['postgres', 'orca'] LOOP
    PERFORM set_config('pg_orca.enable_orca', (engine = 'orca')::text, true);
    FOREACH variant IN ARRAY ARRAY['scalar', 'scalar_blocking', 'exists_value', 'exists_predicate',
        'exists_value_offset', 'exists_predicate_offset'] LOOP
      FOREACH scenario IN ARRAY ARRAY['empty', 'one', 'two', 'late_error', 'early_error', 'second_error'] LOOP
        condition := CASE scenario WHEN 'empty' THEN 'k > 3' WHEN 'one' THEN 'k = 1'
          WHEN 'two' THEN 'k <= 2' WHEN 'late_error' THEN 'true'
          WHEN 'early_error' THEN 'k = 3' ELSE 'k <> 2' END;
        query := 'SELECT 10 / divisor AS value FROM demand_subquery WHERE k >= 1 AND (' || condition || ')';
        IF variant LIKE 'exists_predicate%' THEN
          query := query || ' AND 10 / divisor > 0';
        END IF;
        IF variant LIKE '%_offset' THEN
          -- OFFSET 0 is an EXISTS simplification fence in the PG planner.
          -- Use a nonblocking index path so target demand is observed directly.
          query := query || ' OFFSET 0';
        ELSE
          query := query || ' ORDER BY ' || CASE WHEN variant = 'scalar_blocking' THEN 'value' ELSE 'k' END;
        END IF;
        query := 'SELECT ' || CASE WHEN variant LIKE 'exists_%' THEN 'EXISTS' ELSE '' END
          || '(' || query || ') AS value FROM demand_subquery_outer WHERE k = 1';
        plan_text := '';
        FOR plan_line IN EXECUTE 'EXPLAIN (VERBOSE, COSTS ON) ' || query LOOP
          plan_text := plan_text || plan_line || E'\n';
        END LOOP;
        blocking := position('Sort' IN plan_text) > 0;
        division_present := position('10 /' IN plan_text) > 0;
        IF (engine = 'orca' AND position('Optimizer: pg_orca' IN plan_text) = 0)
            OR (engine = 'postgres' AND position('Optimizer: pg_orca' IN plan_text) > 0)
            OR position('using demand_subquery_pkey' IN plan_text) = 0
            OR position('Plan 1' IN plan_text) = 0
            OR (variant = 'scalar' AND scenario = 'late_error' AND blocking)
            OR (variant LIKE '%_offset' AND (blocking OR position('Backward' IN plan_text) > 0))
            OR (variant = 'exists_value' AND division_present)
            OR (variant LIKE 'scalar%' AND NOT division_present)
            OR (variant = 'scalar_blocking' AND scenario IN ('two', 'late_error', 'second_error')
                AND NOT blocking) THEN
          RAISE EXCEPTION 'unexpected demand plan: %', plan_text;
        END IF;
        result_value := NULL;
        result_state := '00000';
        BEGIN
          EXECUTE query INTO STRICT result_value;
        EXCEPTION WHEN division_by_zero OR cardinality_violation THEN
          result_state := SQLSTATE;
        END;
        expected_state := CASE
          WHEN (variant IN ('exists_value', 'exists_value_offset') AND NOT division_present)
            OR scenario IN ('empty', 'one') THEN '00000'
          WHEN scenario = 'early_error' THEN '22012'
          WHEN variant LIKE 'exists_%' THEN
            CASE WHEN blocking AND scenario IN ('late_error', 'second_error') THEN '22012' ELSE '00000' END
          WHEN scenario = 'second_error' OR (scenario = 'late_error' AND blocking) THEN '22012'
          ELSE '21000' END;
        expected_value := CASE WHEN expected_state <> '00000' THEN NULL
          WHEN variant LIKE 'exists_%' THEN (scenario <> 'empty')::text
          WHEN scenario = 'empty' THEN NULL ELSE '10' END;
        IF result_state <> expected_state OR result_value IS DISTINCT FROM expected_value THEN
          RAISE EXCEPTION 'unexpected subquery demand: %, %, %, %, %, %, %',
            engine, variant, scenario, result_state, result_value, expected_state, expected_value;
        END IF;
        RAISE NOTICE '%', json_build_object('engine', engine, 'variant', variant,
          'scenario', scenario, 'sqlstate', result_state, 'value', result_value,
          'blocking', blocking, 'division_present', division_present, 'plan', plan_text);
      END LOOP;
    END LOOP;
  END LOOP;
END
$probe$;
ROLLBACK;
