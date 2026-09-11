# pgorca_rule_audit

Static capability audit for a directory of MONSOON/WeTune DSL rule files.
The tool uses pgorca's production parser and template IR; it does not identify
rules by text or encode individual rewrites.

Run the production-parser feature check with
`python3 tools/pgorca_rule_audit/test_features.py build-ninja/pgorca_rule_audit`.

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
- `rule_graph.json`: admitted rules, target-path anchored static edges, and
  unresolved `Input` anchors which Bots/workloads may later turn into observed
  edges. Each node also has `template_features` computed directly from the
  production RuleIR: source/target nodes (including Input), maximum depth
  (root=1), Input occurrences, positional symbol-slot occurrences, total
  constraint occurrences and distinct constraint kinds. These are structural
  counts, not measured runtime check costs; repeated symbols are not deduplicated
  in slot counts.
- `rule_graph.dot`: the same graph rendered as Graphviz input, including
  parallel edges and dashed unresolved `Input` anchors.

`supported_static` means that every source operator has a matcher, every target
operator has an instantiator, the source root has a DSL shell, and every
constraint has a checker. It does not mean that a particular SQL expression
will match, satisfy live metadata constraints, or produce a usable plan.

Merge workload-observed `rule_edge` records into a persistent graph and render
them as blue DOT edges with:

```sh
python3 test/dsl/merge_rule_graph.py build/rule-audit/rule_graph.json \
  build/workload.log --output build/rule_graph.merged.json \
  --dot build/rule_graph.merged.dot
```

CBO origin edges in the statistics-experiment stream now include all evaluated
attempts, not only candidates ready for insertion. Ordinary fixed-buffer
diagnostics retain the previous root/ready-only scope to avoid truncating final
Memo provenance. `candidate_status` distinguishes attempts; unsuccessful evaluations
use `relation=binding_observed`, with the producer's original relation retained
as `producer_relation`. They are not successful rewrite or final-plan edges.
`dst_candidate_sequence` joins an edge to its query-local candidate record
(zero without the statistics-experiment stream). The outcome's version and
edge count, plus consecutive `binding_edge_sequence` values, allow
`profile_rule_candidates.binding_origin_evidence` to reject incomplete traces.

Only the extracted binding is traversed, including its child expressions; no
other Memo alternatives or statistics are derived. `dst_binding_path` is an
actual expression-tree position. For children, `dst_source_path` is null because
the corresponding DSL-template position has not been established. The merger
keeps these positions as separate parallel edges and retains attempt status
counts. Direct registered producers are observed; transitive native rewrites,
duplicate alternative producers, and all causal enabling conditions are not
yet reconstructed.
