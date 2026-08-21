#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
RULE_FILE="${DSL_RULE_FILE:-$SCRIPT_DIR/rules/repeated_insub.rules}"
OUTPUT_DIR="${DSL_E2E_OUTPUT_DIR:-$REPO_ROOT/build/dsl-e2e}"
PG_CONFIG="${PG_CONFIG:-$(command -v pg_config || true)}"
PORT="${DSL_E2E_PORT:-55439}"

fail()
{
    echo "DSL E2E FAILED: $*" >&2
    exit 1
}

if [[ -z "$PG_CONFIG" || ! -x "$PG_CONFIG" ]]; then
    fail "PG_CONFIG must point to an executable pg_config"
fi
if [[ ! -r "$RULE_FILE" ]]; then
    fail "rule fixture not found: $RULE_FILE"
fi
if [[ ! "$PORT" =~ ^[0-9]+$ ]] || (( PORT < 1 || PORT > 65535 )); then
    fail "DSL_E2E_PORT must be an integer between 1 and 65535"
fi

PG_BINDIR="$($PG_CONFIG --bindir)"
for program in initdb pg_ctl psql; do
    if [[ ! -x "$PG_BINDIR/$program" ]]; then
        fail "required PostgreSQL program not found: $PG_BINDIR/$program"
    fi
done

mkdir -p "$OUTPUT_DIR"
DSL_E2E_ROOT="$(mktemp -d /tmp/pgorca-dsl-e2e.XXXXXX)"
DATA_DIR="$DSL_E2E_ROOT/data"
SOCKET_DIR="$DSL_E2E_ROOT/socket"
SERVER_LOG="$OUTPUT_DIR/postgresql.log"
SERVER_STARTED=0
mkdir -p "$SOCKET_DIR"
: >"$SERVER_LOG"

cleanup()
{
    if [[ $SERVER_STARTED -eq 1 ]]; then
        "$PG_BINDIR/pg_ctl" -D "$DATA_DIR" stop -m fast >/dev/null 2>&1 || true
    fi
    if [[ "${DSL_E2E_KEEP_TMP:-0}" = "1" ]]; then
        echo "Preserving DSL E2E temporary directory: $DSL_E2E_ROOT" >&2
    else
        rm -rf -- "$DSL_E2E_ROOT"
    fi
}
trap cleanup EXIT

"$PG_BINDIR/initdb" -D "$DATA_DIR" --no-locale --encoding=UTF8 --auth=trust \
    >"$OUTPUT_DIR/initdb.log"

MONSOON_DSL_RULES="$RULE_FILE" \
    "$PG_BINDIR/pg_ctl" -D "$DATA_DIR" -l "$SERVER_LOG" \
    -o "-c listen_addresses='' -c logging_collector=off -k $SOCKET_DIR -p $PORT" \
    start
SERVER_STARTED=1

PSQL=(
    "$PG_BINDIR/psql"
    -X
    -v ON_ERROR_STOP=1
    -h "$SOCKET_DIR"
    -p "$PORT"
    -d postgres
)

"${PSQL[@]}" -q -c "
CREATE EXTENSION pg_orca;
CREATE TABLE dsl_insub_outer(id int PRIMARY KEY, v int NOT NULL);
CREATE TABLE dsl_insub_inner(id int PRIMARY KEY);
INSERT INTO dsl_insub_outer VALUES (1,10),(2,20),(3,30);
INSERT INTO dsl_insub_inner VALUES (1),(3);
ANALYZE dsl_insub_outer;
ANALYZE dsl_insub_inner;
"

QUERY="
SELECT id, v
FROM dsl_insub_outer o
WHERE id IN (SELECT id FROM dsl_insub_inner i1)
  AND id IN (SELECT id FROM dsl_insub_inner i2)
ORDER BY id
"

run_explain()
{
    local output_file=$1
    local dsl_enabled=$2
    local disable_native=$3
    local disable_sql=""

    if [[ "$disable_native" = "yes" ]]; then
        disable_sql="DO 'BEGIN PERFORM disable_xform(''CXformSelect2Apply''); END';"
    fi

    "${PSQL[@]}" -q -c "
LOAD 'pg_orca';
SET pg_orca.enable_orca=on;
SET pg_orca.enable_dsl_rule=$dsl_enabled;
$disable_sql
EXPLAIN (COSTS OFF) $QUERY;
" >"$output_file" 2>&1
}

assert_contains()
{
    local file=$1
    local text=$2
    grep -Fq "$text" "$file" || fail "$file does not contain: $text"
}

assert_not_contains()
{
    local file=$1
    local text=$2
    if grep -Fq "$text" "$file"; then
        fail "$file unexpectedly contains: $text"
    fi
}

count_plan_joins()
{
    grep -Ec \
        '^[[:space:]]*(->[[:space:]]*)?(((Hash|Merge)([[:space:]]+(Left|Right|Full|Semi|Anti))?[[:space:]]+Join)|Nested Loop([[:space:]]+(Left|Right|Full|Semi|Anti)[[:space:]]+Join)?)' \
        "$1" || true
}

# Normal ON/OFF comparison: native ORCA first unnests the subqueries, then the
# post-Apply DSL rule removes the duplicate join.
run_explain "$OUTPUT_DIR/dsl-off-native-on.plan" off no
run_explain "$OUTPUT_DIR/dsl-on-native-on.plan" on no

assert_contains "$OUTPUT_DIR/dsl-off-native-on.plan" "Optimizer: pg_orca"
assert_contains "$OUTPUT_DIR/dsl-on-native-on.plan" "Optimizer: pg_orca"

off_join_count="$(count_plan_joins "$OUTPUT_DIR/dsl-off-native-on.plan")"
on_join_count="$(count_plan_joins "$OUTPUT_DIR/dsl-on-native-on.plan")"
if (( off_join_count < 2 )); then
    fail "DSL OFF baseline should contain at least two joins, found $off_join_count"
fi
if (( on_join_count != 1 )); then
    fail "DSL ON plan should contain exactly one join, found $on_join_count"
fi

# Causal control: with the native subquery-to-Apply xform disabled, pg_orca can
# only produce a plan when the loaded DSL rule performs the rewrite itself.
run_explain "$OUTPUT_DIR/dsl-off-native-off.plan" off yes
run_explain "$OUTPUT_DIR/dsl-on-native-off.plan" on yes

assert_not_contains "$OUTPUT_DIR/dsl-off-native-off.plan" "Optimizer: pg_orca"
assert_contains "$OUTPUT_DIR/dsl-on-native-off.plan" "Optimizer: pg_orca"
causal_join_count="$(count_plan_joins "$OUTPUT_DIR/dsl-on-native-off.plan")"
if (( causal_join_count != 1 )); then
    fail "causal DSL ON plan should contain exactly one join, found $causal_join_count"
fi

# Execute the causally rewritten query and compare it with PostgreSQL's planner.
dsl_rows="$("${PSQL[@]}" -qAt -c "
LOAD 'pg_orca';
SET pg_orca.enable_orca=on;
SET pg_orca.enable_dsl_rule=on;
DO 'BEGIN PERFORM disable_xform(''CXformSelect2Apply''); END';
COPY ($QUERY) TO STDOUT WITH (FORMAT csv);
")"
postgres_rows="$("${PSQL[@]}" -qAt -c "
LOAD 'pg_orca';
SET pg_orca.enable_orca=off;
COPY ($QUERY) TO STDOUT WITH (FORMAT csv);
")"
expected_rows=$'1,10\n3,30'

if [[ "$dsl_rows" != "$postgres_rows" ]]; then
    fail "DSL and PostgreSQL returned different rows"
fi
if [[ "$dsl_rows" != "$expected_rows" ]]; then
    fail "unexpected result rows: $dsl_rows"
fi

# Constraint negative control: the second inner relation differs, so
# TableEq(t1,t2) must reject the rule. Native unnesting is still disabled;
# absence of the pg_orca annotation therefore means the expected fallback.
"${PSQL[@]}" -q -c "
LOAD 'pg_orca';
SET pg_orca.enable_orca=on;
SET pg_orca.enable_dsl_rule=on;
DO 'BEGIN PERFORM disable_xform(''CXformSelect2Apply''); END';
EXPLAIN (COSTS OFF)
SELECT id, v
FROM dsl_insub_outer o
WHERE id IN (SELECT id FROM dsl_insub_inner i1)
  AND id IN (SELECT id FROM dsl_insub_outer i2)
ORDER BY id;
" >"$OUTPUT_DIR/tableeq-negative.plan" 2>&1
assert_not_contains "$OUTPUT_DIR/tableeq-negative.plan" "Optimizer: pg_orca"

echo "DSL E2E passed: OFF=$off_join_count joins, ON=$on_join_count join, causal ON=$causal_join_count join"
echo "Artifacts: $OUTPUT_DIR"
