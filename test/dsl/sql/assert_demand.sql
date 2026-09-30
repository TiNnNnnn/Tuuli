-- Run on an isolated PG/ORCA instance with MONSOON_DSL_RULES pointing to
-- test/dsl/rules/orca_replacements.rules; pass psql -v dsl_policy=<absolute path
-- to test/dsl/rules/isolate_max_one_row_assert.policy> -v ON_ERROR_STOP=1.
-- Compare the direct and DSL physical implementations, not arbitrary SQL
-- error schedules: PG evaluates this target below Sort, ORCA above Sort.
BEGIN;
CREATE EXTENSION IF NOT EXISTS pg_orca;
LOAD 'pg_orca';
SET LOCAL statement_timeout = '1min';
SET LOCAL max_parallel_workers_per_gather = 0;
SET LOCAL jit = off;
SET LOCAL pg_orca.enable_assert_maxonerow = on;
SET LOCAL pg_orca.dsl_rule_policy_path = :'dsl_policy';
CREATE TEMP TABLE assertion_input(k int PRIMARY KEY, d int);
INSERT INTO assertion_input VALUES (1,1),(2,2),(3,0);
ANALYZE assertion_input;
CREATE TEMP TABLE assertion_outer(k int PRIMARY KEY);
INSERT INTO assertion_outer VALUES (1);
ANALYZE assertion_outer;
DO $probe$
DECLARE
  mode text;
  expression text;
  query text;
  plan text;
  line text;
  state text;
  expected_state text;
  result text;
BEGIN
  FOREACH mode IN ARRAY ARRAY['postgres', 'direct', 'dsl'] LOOP
    PERFORM set_config('pg_orca.enable_orca', (mode <> 'postgres')::text, true);
    PERFORM set_config('pg_orca.enable_dsl_rule', (mode = 'dsl')::text, true);
    IF mode <> 'postgres' THEN
      PERFORM disable_xform('CXformMaxOneRow2Assert');
      PERFORM disable_xform('CXformImplementLeftOuterCorrelatedApply');
      PERFORM disable_xform('CXformImplementInnerCorrelatedApply');
    END IF;
    IF mode = 'dsl' THEN
      PERFORM disable_xform('CXformImplementMaxOneRow');
    END IF;
    FOREACH expression IN ARRAY ARRAY['d', '10 / (d - 1)', '10 / (d - 2)', '10 / d'] LOOP
      query := 'SELECT (SELECT ' || expression || ' FROM assertion_input ORDER BY k) FROM assertion_outer';
      plan := '';
      FOR line IN EXECUTE 'EXPLAIN (VERBOSE, COSTS OFF) ' || query LOOP
        plan := plan || line || E'\n';
      END LOOP;
      IF (mode = 'postgres') <> (position('Optimizer: pg_orca' IN plan) = 0)
          OR position('Sort' IN plan) = 0
          OR (mode <> 'postgres' AND position('Custom Scan (Assert)' IN plan) = 0)
          OR (mode = 'dsl') <> (position('WindowAgg' IN plan) > 0)
          OR (mode <> 'postgres' AND expression <> 'd' AND position('Result' IN plan) = 0) THEN
        RAISE EXCEPTION 'unexpected assertion path: %, %', mode, plan;
      END IF;
      state := '00000';
      BEGIN
        EXECUTE query INTO result;
      EXCEPTION WHEN OTHERS THEN state := SQLSTATE;
      END;
      expected_state := CASE
        WHEN expression = 'd' OR (expression = '10 / d' AND mode <> 'postgres') THEN '21000'
        ELSE '22012' END;
      IF state <> expected_state THEN
        RAISE EXCEPTION 'assertion demand mismatch: %, %, got %, expected %; %',
          mode, expression, state, expected_state, plan;
      END IF;
      RAISE NOTICE '%', json_build_object('mode', mode, 'expression', expression,
        'sqlstate', state, 'plan', plan);
    END LOOP;
  END LOOP;
END
$probe$;
ROLLBACK;
