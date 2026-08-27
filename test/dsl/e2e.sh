#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
RULE_FILE="${DSL_RULE_FILE:-$SCRIPT_DIR/rules/framework.rules}"
SQL_DIR="${DSL_E2E_SQL_DIR:-$SCRIPT_DIR/e2e/sql}"
EXPECT_DIR="${DSL_E2E_EXPECT_DIR:-$SCRIPT_DIR/e2e/expect}"
POLICY_DIR="${DSL_E2E_POLICY_DIR:-$SCRIPT_DIR/rules}"
RESULT_DIR="${DSL_E2E_RESULT_DIR:-$SCRIPT_DIR/e2e/output}"
DIFF_DIR="${DSL_E2E_DIFF_DIR:-$SCRIPT_DIR/e2e/diff}"
ARTIFACT_DIR="${DSL_E2E_OUTPUT_DIR:-$REPO_ROOT/build/dsl-e2e}"
PG_CONFIG="${PG_CONFIG:-$(command -v pg_config || true)}"
PORT="${DSL_E2E_PORT:-55439}"

fail()
{
    echo "DSL E2E FAILED: $*" >&2
    exit 1
}

usage()
{
    cat <<EOF
Usage: $0 [-t CASE]...

  -t, --test CASE  Run one case name or pattern; may be repeated
  -h, --help       Show this help
EOF
}

SELECTED_CASES="${DSL_E2E_CASES:-}"
while (( $# > 0 )); do
    case "$1" in
        -t|--test)
            (( $# >= 2 )) || fail "$1 requires a case name or pattern"
            if [[ -n "$SELECTED_CASES" ]]; then
                SELECTED_CASES+=",$2"
            else
                SELECTED_CASES="$2"
            fi
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            fail "unknown argument: $1"
            ;;
    esac
done

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

mkdir -p "$ARTIFACT_DIR" "$RESULT_DIR" "$DIFF_DIR"
DSL_E2E_ROOT="$(mktemp -d /tmp/pgorca-dsl-e2e.XXXXXX)"
DATA_DIR="$DSL_E2E_ROOT/data"
SOCKET_DIR="$DSL_E2E_ROOT/socket"
SERVER_LOG="$ARTIFACT_DIR/postgresql.log"
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
    >"$ARTIFACT_DIR/initdb.log"

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

"${PSQL[@]}" -q -f "$SQL_DIR/_setup.sql"

CASE_ARGS=()
if [[ -n "$SELECTED_CASES" ]]; then
    CASE_ARGS=(--cases "$SELECTED_CASES")
fi

python3 "$SCRIPT_DIR/run_e2e_cases.py" \
    --psql "$PG_BINDIR/psql" \
    --host "$SOCKET_DIR" \
    --port "$PORT" \
    --sql-dir "$SQL_DIR" \
    --expect-dir "$EXPECT_DIR" \
    --policy-dir "$POLICY_DIR" \
    --result-dir "$RESULT_DIR" \
    --diff-dir "$DIFF_DIR" \
    --artifact-dir "$ARTIFACT_DIR" \
    "${CASE_ARGS[@]}"

echo "Actual output: $RESULT_DIR"
echo "Diffs: $DIFF_DIR"
echo "Detailed artifacts: $ARTIFACT_DIR"
