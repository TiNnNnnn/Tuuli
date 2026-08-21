# pgorca_rule_audit

Static capability audit for a directory of MONSOON/WeTune DSL rule files.
The tool uses pgorca's production parser and template IR; it does not identify
rules by text or encode individual rewrites.

```sh
cmake --build build --target pgorca_rule_audit
build/pgorca_rule_audit /path/to/rules build/rule-audit
```

It writes:

- `coverage.json`: totals, operator/constraint frequencies, per-rule status,
  reason codes, and possible native Xforms inferred from factory patterns.
- `unsupported_features.csv`: unsupported stage/feature counts, suitable for
  prioritizing generic matcher/checker/instantiator work.
- `replacement_candidates.csv`: statically supported rules and possible native
  Xforms. Every row remains `needs_runtime_replacement_test` until the
  native/shadow/negative/replacement E2E matrix proves causality.

`supported_static` means that every source operator has a matcher, every target
operator has an instantiator, the source root has a DSL shell, and every
constraint has a checker. It does not mean that a particular SQL expression
will match, satisfy live metadata constraints, or produce a usable plan.
