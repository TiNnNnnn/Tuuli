#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
RULE_FILE="${DSL_RULE_FILE:-$SCRIPT_DIR/rules/framework.rules}"
OUTPUT_DIR="${DSL_E2E_OUTPUT_DIR:-$REPO_ROOT/build/dsl-e2e}"
PG_CONFIG="${PG_CONFIG:-$(command -v pg_config || true)}"
PORT="${DSL_E2E_PORT:-55439}"
REPLACEMENT_XFORMS="CXformSelect2Apply, CXformProject2Apply"

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
CREATE TABLE dsl_correlated_exists(k int, payload int NOT NULL);
INSERT INTO dsl_correlated_exists VALUES (1,10),(NULL,20),(2,30);
CREATE TABLE dsl_agg_outer(g int, v int);
CREATE TABLE dsl_exists_inner(x int);
CREATE TABLE dsl_dqa(empno int PRIMARY KEY, deptno int NOT NULL);
CREATE TABLE dsl_fk_parent(id int PRIMARY KEY);
CREATE TABLE dsl_fk_child(
    id int PRIMARY KEY,
    parent_id int NOT NULL REFERENCES dsl_fk_parent(id));
CREATE TABLE dsl_fk_nullable_child(
    id int PRIMARY KEY,
    parent_id int REFERENCES dsl_fk_parent(id));
INSERT INTO dsl_agg_outer VALUES (1,10),(1,20),(2,NULL),(3,5);
INSERT INTO dsl_exists_inner VALUES (5),(20);
INSERT INTO dsl_dqa VALUES (1,10),(2,10),(3,20);
INSERT INTO dsl_fk_parent VALUES (10),(20);
INSERT INTO dsl_fk_child VALUES (1,10),(2,10),(3,20);
INSERT INTO dsl_fk_nullable_child VALUES (1,10),(2,NULL);
ANALYZE dsl_insub_outer;
ANALYZE dsl_insub_inner;
ANALYZE dsl_correlated_exists;
ANALYZE dsl_agg_outer;
ANALYZE dsl_exists_inner;
ANALYZE dsl_dqa;
ANALYZE dsl_fk_parent;
ANALYZE dsl_fk_child;
ANALYZE dsl_fk_nullable_child;
"

REPEATED_IN_QUERY="
SELECT id, v
FROM dsl_insub_outer o
WHERE id IN (SELECT id FROM dsl_insub_inner i1)
  AND id IN (SELECT id FROM dsl_insub_inner i2)
ORDER BY id
"

SELF_IN_QUERY="
SELECT id, v
FROM dsl_insub_outer o
WHERE id IN (SELECT id FROM dsl_insub_outer i)
  AND v > 10
ORDER BY id
"

CORRELATED_EXISTS_QUERY="
SELECT payload
FROM dsl_correlated_exists o
WHERE EXISTS (
    SELECT 1
    FROM dsl_correlated_exists i
    WHERE i.k = o.k)
ORDER BY payload
"

AGG_HAVING_QUERY="
SELECT g, max(v) AS max_v
FROM dsl_agg_outer
GROUP BY g
HAVING max(v) > 5
ORDER BY g
"

AGG_EXISTS_QUERY="
SELECT g, max(v) AS max_v
FROM dsl_agg_outer
GROUP BY g
HAVING max(v) > 5
   AND EXISTS (
       SELECT x + 1
       FROM dsl_exists_inner
       WHERE x = max(v))
ORDER BY g
"

DQA_QUERY="
SELECT empno, count(*), count(DISTINCT deptno)
FROM dsl_dqa
GROUP BY empno
ORDER BY empno
"

DERIVED_FK_LEFT_JOIN_QUERY="
SELECT c.parent_id, p.id
FROM (
    SELECT parent_id
    FROM dsl_fk_child
    WHERE id > 0
    GROUP BY parent_id
) AS c
LEFT JOIN dsl_fk_parent AS p ON c.parent_id = p.id
ORDER BY c.parent_id
"

NULLABLE_FK_LEFT_JOIN_QUERY="
SELECT c.id, p.id
FROM dsl_fk_nullable_child AS c
LEFT JOIN dsl_fk_parent AS p ON c.parent_id = p.id
ORDER BY c.id
"

UNION_QUERY="
SELECT id
FROM (
    SELECT id FROM dsl_insub_outer WHERE id <= 2
    UNION ALL
    SELECT id FROM dsl_insub_outer WHERE id >= 2
) AS union_rows
ORDER BY id
"

UNION_DISTINCT_QUERY="
SELECT id
FROM (
    SELECT id FROM dsl_insub_outer WHERE id <= 2
    UNION
    SELECT id FROM dsl_insub_outer WHERE id >= 2
) AS union_rows
ORDER BY id
"

SORT_QUERY="
SELECT id
FROM dsl_insub_outer
ORDER BY id
"

NESTED_SORT_QUERY="
SELECT id, v
FROM (
    SELECT id, v
    FROM dsl_insub_outer
    ORDER BY v
) AS ordered_inner
ORDER BY id
"

ORDER_LIMIT_QUERY="
SELECT id
FROM dsl_insub_outer
ORDER BY id
LIMIT 2 OFFSET 1
"

run_explain()
{
    local output_file=$1
    local dsl_enabled=$2
    local native_enabled=$3
    local query=$4
    local trace_xforms=${5:-off}
    local native_sql="SET pg_orca.dsl_only_xforms='';"

    if [[ "$native_enabled" = "off" ]]; then
        native_sql="SET pg_orca.dsl_only_xforms='$REPLACEMENT_XFORMS';"
    fi

    "${PSQL[@]}" -q -c "
LOAD 'pg_orca';
SET pg_orca.enable_orca=on;
SET pg_orca.enable_dsl_rule=$dsl_enabled;
$native_sql
SET optimizer_print_xform=$trace_xforms;
SET optimizer_print_xform_results=$trace_xforms;
SET pg_orca.trace_dsl_rule=$trace_xforms;
SET client_min_messages=log;
EXPLAIN (COSTS OFF) $query;
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

assert_xform_produced_alternative()
{
    local file=$1
    local xform=$2

    awk -v xform="$xform" '
        index($0, "Xform: " xform) {
            in_xform = 1
            in_alternatives = 0
            next
        }
        in_xform && index($0, "TRACE,\"Xform:") {
            in_xform = 0
        }
        in_xform && /^Alternatives:/ {
            in_alternatives = 1
            next
        }
        in_xform && in_alternatives && /^0:/ {
            found = 1
        }
        END { exit(found ? 0 : 1) }
    ' "$file" || fail "$xform produced no alternative in $file"
}

count_plan_joins()
{
    grep -Ec \
        '^[[:space:]]*(->[[:space:]]*)?(((Hash|Merge)([[:space:]]+(Left|Right|Full|Semi|Anti))?[[:space:]]+Join)|Nested Loop([[:space:]]+(Left|Right|Full|Semi|Anti)[[:space:]]+Join)?)' \
        "$1" || true
}

assert_same_rows()
{
    local case_name=$1
    local query=$2
    local expected=$3
    local native_enabled=$4
    local native_sql="SET pg_orca.dsl_only_xforms='';"
    local dsl_rows
    local postgres_rows

    if [[ "$native_enabled" = "off" ]]; then
        native_sql="SET pg_orca.dsl_only_xforms='$REPLACEMENT_XFORMS';"
    fi

    dsl_rows="$("${PSQL[@]}" -qAt -c "
LOAD 'pg_orca';
SET pg_orca.enable_orca=on;
SET pg_orca.enable_dsl_rule=on;
$native_sql
COPY ($query) TO STDOUT WITH (FORMAT csv);
")"
    postgres_rows="$("${PSQL[@]}" -qAt -c "
LOAD 'pg_orca';
SET pg_orca.enable_orca=off;
COPY ($query) TO STDOUT WITH (FORMAT csv);
")"

    if [[ "$dsl_rows" != "$postgres_rows" ]]; then
        fail "$case_name: DSL and PostgreSQL returned different rows"
    fi
    if [[ "$dsl_rows" != "$expected" ]]; then
        fail "$case_name: unexpected result rows: $dsl_rows"
    fi
}

# Normal ON/OFF comparison: native ORCA first unnests the subqueries, then the
# post-Apply DSL rule removes the duplicate join.
run_explain "$OUTPUT_DIR/dsl-off-native-on.plan" off on "$REPEATED_IN_QUERY"
run_explain "$OUTPUT_DIR/dsl-on-native-on.plan" on on "$REPEATED_IN_QUERY"

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
run_explain "$OUTPUT_DIR/dsl-off-native-off.plan" off off "$REPEATED_IN_QUERY"
run_explain "$OUTPUT_DIR/dsl-on-native-off.plan" on off "$REPEATED_IN_QUERY"

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
SET pg_orca.dsl_only_xforms='$REPLACEMENT_XFORMS';
COPY ($REPEATED_IN_QUERY) TO STDOUT WITH (FORMAT csv);
")"
postgres_rows="$("${PSQL[@]}" -qAt -c "
LOAD 'pg_orca';
SET pg_orca.enable_orca=off;
COPY ($REPEATED_IN_QUERY) TO STDOUT WITH (FORMAT csv);
")"
expected_rows=$'1,10\n3,30'

if [[ "$dsl_rows" != "$postgres_rows" ]]; then
    fail "DSL and PostgreSQL returned different rows"
fi
if [[ "$dsl_rows" != "$expected_rows" ]]; then
    fail "unexpected result rows: $dsl_rows"
fi

# A different real rule exercises the same InSub matcher with a one-node source
# and an eliminating target. This guards against implementing only the repeated
# IN tree shape. The residual v > 10 predicate must survive elimination.
run_explain "$OUTPUT_DIR/self-in-off-native-on.plan" off on "$SELF_IN_QUERY"
run_explain "$OUTPUT_DIR/self-in-on-native-on.plan" on on "$SELF_IN_QUERY" on
assert_contains "$OUTPUT_DIR/self-in-off-native-on.plan" "Optimizer: pg_orca"
assert_contains "$OUTPUT_DIR/self-in-on-native-on.plan" "Optimizer: pg_orca"
assert_xform_produced_alternative \
    "$OUTPUT_DIR/self-in-on-native-on.plan" "CXformDSLRule_InSub"
self_off_join_count="$(count_plan_joins "$OUTPUT_DIR/self-in-off-native-on.plan")"
self_on_join_count="$(count_plan_joins "$OUTPUT_DIR/self-in-on-native-on.plan")"
if (( self_off_join_count < 1 )); then
    fail "self-IN DSL OFF baseline should contain a join"
fi
if (( self_on_join_count != 0 )); then
    fail "self-IN DSL ON should eliminate the join, found $self_on_join_count"
fi

run_explain "$OUTPUT_DIR/self-in-off-native-off.plan" off off "$SELF_IN_QUERY"
run_explain "$OUTPUT_DIR/self-in-on-native-off.plan" on off "$SELF_IN_QUERY" on
assert_not_contains "$OUTPUT_DIR/self-in-off-native-off.plan" "Optimizer: pg_orca"
assert_contains "$OUTPUT_DIR/self-in-on-native-off.plan" "Optimizer: pg_orca"
assert_xform_produced_alternative \
    "$OUTPUT_DIR/self-in-on-native-off.plan" "CXformDSLRule_Select"
assert_contains "$OUTPUT_DIR/self-in-on-native-off.plan" "stage=applied bindings="
assert_contains "$OUTPUT_DIR/self-in-on-native-off.plan" "Rule: InSubFilter"
assert_contains "$OUTPUT_DIR/self-in-on-native-off.plan" "Generated:"
# Machine-readable differential trace. Rule ids are physical lines in the
# loaded file, so framework.rules:8 aligns with WeTune's loadBank ids.
assert_contains "$OUTPUT_DIR/self-in-on-native-off.plan" \
    'DSL_TRACE {"kind":"application","engine":"pgorca"'
assert_contains "$OUTPUT_DIR/self-in-on-native-off.plan" \
    '"rule_id":8,"status":"applied"'
if (( $(count_plan_joins "$OUTPUT_DIR/self-in-on-native-off.plan") != 0 )); then
    fail "causal self-IN DSL ON should eliminate the join"
fi
assert_same_rows "self-IN" "$SELF_IN_QUERY" $'2,20\n3,30' off

# WeTune canonicalizes a correlated EXISTS equality to InSubFilter, whereas
# native ORCA keeps it as EXISTS/LeftSemiApply. The Apply-stage representation
# adapter must therefore run after native Select2Apply. The
# nullable row is the semantic guard: dropping EXISTS without retaining the
# implied IS NOT NULL predicate would incorrectly return payload=20.
run_explain "$OUTPUT_DIR/correlated-exists-off.plan" off on \
    "$CORRELATED_EXISTS_QUERY"
run_explain "$OUTPUT_DIR/correlated-exists-on.plan" on on \
    "$CORRELATED_EXISTS_QUERY" on
assert_contains "$OUTPUT_DIR/correlated-exists-off.plan" "Optimizer: pg_orca"
assert_contains "$OUTPUT_DIR/correlated-exists-on.plan" "Optimizer: pg_orca"
assert_xform_produced_alternative \
    "$OUTPUT_DIR/correlated-exists-on.plan" "CXformDSLRule_Exists"
assert_contains "$OUTPUT_DIR/correlated-exists-on.plan" \
    '"rule_id":8,"status":"applied"'
assert_contains "$OUTPUT_DIR/correlated-exists-on.plan" "IS NULL"
if (( $(count_plan_joins "$OUTPUT_DIR/correlated-exists-on.plan") != 0 )); then
    fail "correlated EXISTS DSL rewrite should eliminate the join"
fi
assert_same_rows "correlated EXISTS adapter" "$CORRELATED_EXISTS_QUERY" \
    $'10\n30' on

# Agg/HAVING is an identity-shaped framework probe: the observable plan need
# not change, so xform trace proves that generic match/check/instantiate ran.
run_explain "$OUTPUT_DIR/agg-having-off.plan" off on "$AGG_HAVING_QUERY" on
run_explain "$OUTPUT_DIR/agg-having-on.plan" on on "$AGG_HAVING_QUERY" on
assert_contains "$OUTPUT_DIR/agg-having-off.plan" "Optimizer: pg_orca"
assert_contains "$OUTPUT_DIR/agg-having-on.plan" "Optimizer: pg_orca"
assert_not_contains "$OUTPUT_DIR/agg-having-off.plan" "CXformDSLRule_Select"
assert_xform_produced_alternative \
    "$OUTPUT_DIR/agg-having-on.plan" "CXformDSLRule_Select"
assert_same_rows "Agg/HAVING" "$AGG_HAVING_QUERY" $'1,20' on

# The real Exists(Agg,Proj) corpus rule crosses matcher/instantiator boundaries.
# With native unnesting disabled, only the Select-stage DSL representation can
# make the query optimizable; the sibling HAVING conjunct is also preserved.
run_explain "$OUTPUT_DIR/agg-exists-off-native-on.plan" off on "$AGG_EXISTS_QUERY"
run_explain "$OUTPUT_DIR/agg-exists-on-native-on.plan" on on "$AGG_EXISTS_QUERY"
assert_contains "$OUTPUT_DIR/agg-exists-off-native-on.plan" "Optimizer: pg_orca"
assert_contains "$OUTPUT_DIR/agg-exists-on-native-on.plan" "Optimizer: pg_orca"
run_explain "$OUTPUT_DIR/agg-exists-off-native-off.plan" off off "$AGG_EXISTS_QUERY"
run_explain "$OUTPUT_DIR/agg-exists-on-native-off.plan" on off "$AGG_EXISTS_QUERY" on
assert_not_contains "$OUTPUT_DIR/agg-exists-off-native-off.plan" "Optimizer: pg_orca"
assert_contains "$OUTPUT_DIR/agg-exists-on-native-off.plan" "Optimizer: pg_orca"
assert_xform_produced_alternative \
    "$OUTPUT_DIR/agg-exists-on-native-off.plan" "CXformDSLRule_Select"
assert_same_rows "Agg/HAVING/EXISTS" "$AGG_EXISTS_QUERY" $'1,20' off

# WeTune's inner Proj* for a DISTINCT aggregate is virtual in ORCA: the live
# GbAgg carries IsDistinct on its scalar aggregate instead. The trace proves
# that the ordinary Proj* rule produced the flag-cleared alternative; row
# comparison checks the representation adapter end to end.
run_explain "$OUTPUT_DIR/dqa-off.plan" off on "$DQA_QUERY" on
run_explain "$OUTPUT_DIR/dqa-on.plan" on on "$DQA_QUERY" on
assert_not_contains "$OUTPUT_DIR/dqa-off.plan" "CXformDSLRule_Agg"
assert_xform_produced_alternative \
    "$OUTPUT_DIR/dqa-on.plan" "CXformDSLRule_Agg"
assert_contains "$OUTPUT_DIR/dqa-on.plan" \
    "Rule: Proj*<a0 s0>(Input<t0>)"
assert_contains "$OUTPUT_DIR/dqa-on.plan" '"rule_id":43,"status":"applied"'
assert_same_rows "DISTINCT aggregate adapter" "$DQA_QUERY" \
    $'1,1,1\n2,1,1\n3,1,1' on

# Reference must follow a bound join key through GbAgg(Select(Get)) to its
# unique owning base table. The NOT NULL check remains independent, so nullable
# foreign keys cannot use this LeftJoin -> InnerJoin rule.
run_explain "$OUTPUT_DIR/derived-fk-leftjoin-off.plan" off on \
    "$DERIVED_FK_LEFT_JOIN_QUERY" on
run_explain "$OUTPUT_DIR/derived-fk-leftjoin-on.plan" on on \
    "$DERIVED_FK_LEFT_JOIN_QUERY" on
assert_not_contains "$OUTPUT_DIR/derived-fk-leftjoin-off.plan" \
    "CXformDSLRule_LeftJoin"
assert_xform_produced_alternative \
    "$OUTPUT_DIR/derived-fk-leftjoin-on.plan" "CXformDSLRule_LeftJoin"
assert_contains "$OUTPUT_DIR/derived-fk-leftjoin-on.plan" \
    "Rule: LeftJoin<a0 a1>(Input<t0>,Input<t1>)"
assert_contains "$OUTPUT_DIR/derived-fk-leftjoin-on.plan" \
    '"rule_id":48,"status":"applied"'
assert_same_rows "derived FK LeftJoin to InnerJoin" \
    "$DERIVED_FK_LEFT_JOIN_QUERY" $'10,10\n20,20' on

run_explain "$OUTPUT_DIR/nullable-fk-leftjoin-on.plan" on on \
    "$NULLABLE_FK_LEFT_JOIN_QUERY" on
assert_not_contains "$OUTPUT_DIR/nullable-fk-leftjoin-on.plan" \
    '"rule_id":48,"status":"applied"'
assert_same_rows "nullable FK LeftJoin guard" \
    "$NULLABLE_FK_LEFT_JOIN_QUERY" $'1,10\n2,' on

# Union is symbol-free in the DSL but ORCA carries an ordered output/input
# column map on the logical operator. The data rule swaps two branches over the
# same base relation; a non-empty alternative proves the generic shell,
# matcher, constraint checker and mapping-aware instantiator all participated.
run_explain "$OUTPUT_DIR/union-off.plan" off on "$UNION_QUERY" on
run_explain "$OUTPUT_DIR/union-on.plan" on on "$UNION_QUERY" on
assert_contains "$OUTPUT_DIR/union-off.plan" "Optimizer: pg_orca"
assert_contains "$OUTPUT_DIR/union-on.plan" "Optimizer: pg_orca"
assert_not_contains "$OUTPUT_DIR/union-off.plan" "CXformDSLRule_UnionAll"
assert_xform_produced_alternative \
    "$OUTPUT_DIR/union-on.plan" "CXformDSLRule_UnionAll"
assert_same_rows "Union/UnionAll" "$UNION_QUERY" $'1\n2\n2\n3' on

run_explain "$OUTPUT_DIR/union-distinct-off.plan" off on "$UNION_DISTINCT_QUERY" on
run_explain "$OUTPUT_DIR/union-distinct-on.plan" on on "$UNION_DISTINCT_QUERY" on
assert_contains "$OUTPUT_DIR/union-distinct-off.plan" "Optimizer: pg_orca"
assert_contains "$OUTPUT_DIR/union-distinct-on.plan" "Optimizer: pg_orca"
assert_not_contains "$OUTPUT_DIR/union-distinct-off.plan" "CXformDSLRule_Union"
assert_xform_produced_alternative \
    "$OUTPUT_DIR/union-distinct-on.plan" "CXformDSLRule_Union"
assert_same_rows "Union/Union distinct" "$UNION_DISTINCT_QUERY" $'1\n2\n3' on

# Sort and Limit are not independent logical operators in ORCA. These probes
# prove that the shared CLogicalLimit shell sees both the count-less Sort view
# and the fused Limit(Sort(...)) view, and that target reconstruction remains
# executable with identical rows.
run_explain "$OUTPUT_DIR/sort-off.plan" off on "$SORT_QUERY" on
run_explain "$OUTPUT_DIR/sort-on.plan" on on "$SORT_QUERY" on
assert_not_contains "$OUTPUT_DIR/sort-off.plan" "CXformDSLRule_Limit"
assert_xform_produced_alternative \
    "$OUTPUT_DIR/sort-on.plan" "CXformDSLRule_Limit"
assert_contains "$OUTPUT_DIR/sort-on.plan" "Rule: SortAsc"
assert_same_rows "Sort adapter" "$SORT_QUERY" $'1\n2\n3' on

run_explain "$OUTPUT_DIR/nested-sort-off.plan" off on "$NESTED_SORT_QUERY" on
run_explain "$OUTPUT_DIR/nested-sort-on.plan" on on "$NESTED_SORT_QUERY" on
assert_not_contains "$OUTPUT_DIR/nested-sort-off.plan" "CXformDSLRule_Limit"
assert_xform_produced_alternative \
    "$OUTPUT_DIR/nested-sort-on.plan" "CXformDSLRule_Limit"
assert_contains "$OUTPUT_DIR/nested-sort-on.plan" \
    "Rule: SortAsc<a1>(SortAsc<a0>(Input<t0>))"
assert_contains "$OUTPUT_DIR/nested-sort-on.plan" "stage=applied bindings="
assert_same_rows "Nested Sort elimination" "$NESTED_SORT_QUERY" \
    $'1,10\n2,20\n3,30' on

run_explain "$OUTPUT_DIR/order-limit-off.plan" off on "$ORDER_LIMIT_QUERY" on
run_explain "$OUTPUT_DIR/order-limit-on.plan" on on "$ORDER_LIMIT_QUERY" on
assert_not_contains "$OUTPUT_DIR/order-limit-off.plan" "CXformDSLRule_Limit"
assert_xform_produced_alternative \
    "$OUTPUT_DIR/order-limit-on.plan" "CXformDSLRule_Limit"
assert_contains "$OUTPUT_DIR/order-limit-on.plan" "Rule: Limit"
assert_contains "$OUTPUT_DIR/order-limit-on.plan" "stage=applied bindings="
assert_same_rows "Order/Limit fused adapter" "$ORDER_LIMIT_QUERY" $'2\n3' on

# Constraint negative control: the second inner relation differs, so
# TableEq(t1,t2) must reject the rule. Native unnesting is still disabled;
# absence of the pg_orca annotation therefore means the expected fallback.
"${PSQL[@]}" -q -c "
LOAD 'pg_orca';
SET pg_orca.enable_orca=on;
SET pg_orca.enable_dsl_rule=on;
SET pg_orca.dsl_only_xforms='$REPLACEMENT_XFORMS';
EXPLAIN (COSTS OFF)
SELECT id, v
FROM dsl_insub_outer o
WHERE id IN (SELECT id FROM dsl_insub_inner i1)
  AND id IN (SELECT id FROM dsl_insub_outer i2)
ORDER BY id;
" >"$OUTPUT_DIR/tableeq-negative.plan" 2>&1
assert_not_contains "$OUTPUT_DIR/tableeq-negative.plan" "Optimizer: pg_orca"

echo "DSL E2E passed: repeated-IN, self-IN, correlated EXISTS, Agg/HAVING, Agg/HAVING/EXISTS, DISTINCT aggregate, derived-FK LeftJoin, Union, Union*, Sort elimination, and fused Order/Limit"
echo "Repeated-IN joins: OFF=$off_join_count, ON=$on_join_count, causal ON=$causal_join_count"
echo "Artifacts: $OUTPUT_DIR"
