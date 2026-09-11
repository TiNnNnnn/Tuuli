# Pending Boolean quantifier regressions

These use the tables from `test/dsl/e2e/sql/_setup.sql`. Expected rows are the
PostgreSQL reference, not an accepted current ORCA result. They are outside
automatic E2E discovery until the issues are fixed, not passing regression tests.

On 2026-09-10 the current translator asserted that a quantified SubLink testexpr
is an OpExpr. PostgreSQL folds Boolean constant equality/inequality to a Boolean
PARAM_SUBLINK or NOT(PARAM_SUBLINK), so these queries fall back before DSL runs.

A trial restoring the equivalent Boolean comparison admitted these inputs, but
the combined ALL value query then returned NULL instead of TRUE for empty inner
sets, and NULL instead of FALSE for two mixed-NULL comparisons. The DSL-enabled
result differed from PostgreSQL; native and DSL plan shapes were alike, but this
does not establish the downstream bug's precise owner. The combined ANY query
timed out after 60 seconds with DSL enabled. The trial translator change was
fully reverted and rebuilt: accepting previously rejected inputs is not safe
without validating downstream value semantics.

Local evidence: `output/boolean-quantifier-regression-v1/` plus the E2E output/diff
files of the same case names. `compute-correlated-v2-run` used the withdrawn
translator and is exploratory evidence only, not a mainline performance result.
Do not copy the observed incorrect values into the expectations.
