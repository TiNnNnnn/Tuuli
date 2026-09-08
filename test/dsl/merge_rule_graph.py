#!/usr/bin/env python3
"""Merge observed DSL rule edges into a persistent static rule graph."""

from __future__ import annotations

import argparse
import json
import sys
from copy import deepcopy
from pathlib import Path
from typing import Any, Iterable

from compare_rule_traces import read_records


EDGE_FIELDS = (
    "src_rule",
    "dst_rule",
    "target_path",
    "path_kind",
    "scheduler",
    "evidence",
    "relation",
)


def edge_key(edge: dict[str, Any]) -> tuple[Any, ...]:
    return tuple(edge.get(field) for field in EDGE_FIELDS)


def merge_graph(
    base: dict[str, Any], records: Iterable[dict[str, Any]]
) -> dict[str, Any]:
    graph = deepcopy(base)
    nodes = {
        node.get("rule_hash")
        for node in graph.get("nodes", [])
        if isinstance(node, dict)
    }
    edges = graph.setdefault("edges", [])
    indexed = {
        edge_key(edge): edge
        for edge in edges
        if isinstance(edge, dict) and edge.get("evidence") == "runtime_observed"
    }

    for record in records:
        if record.get("kind") != "rule_edge" or record.get("engine") != "pgorca":
            continue
        src = record.get("src_rule")
        dst = record.get("dst_rule")
        path = record.get("target_path")
        if src not in nodes or dst not in nodes:
            raise ValueError(f"runtime edge references unknown rule: {src} -> {dst}")
        if not isinstance(path, str) or not path:
            raise ValueError("runtime edge has no target_path")

        observed = {
            "src_rule": src,
            "dst_rule": dst,
            "target_path": path,
            "path_kind": record.get("path_kind", "instantiated_expression"),
            "scheduler": record.get("scheduler", "unknown"),
            "evidence": "runtime_observed",
            "relation": record.get("relation", "followed_by"),
        }
        key = edge_key(observed)
        edge = indexed.get(key)
        if edge is None:
            edge = observed
            edge["observations"] = 0
            edge["binding_path_counts"] = {}
            edges.append(edge)
            indexed[key] = edge
        edge["observations"] = int(edge.get("observations", 0)) + 1
        binding = record.get("binding_path")
        if isinstance(binding, str) and binding:
            counts = edge.setdefault("binding_path_counts", {})
            counts[binding] = int(counts.get(binding, 0)) + 1

    graph["schema_version"] = max(2, int(graph.get("schema_version", 1)))
    return graph


def dot_escape(value: Any) -> str:
    return str(value).replace("\\", "\\\\").replace('"', '\\"')


def render_dot(graph: dict[str, Any]) -> str:
    lines = ["digraph dsl_rules {", "  rankdir=LR;"]
    for node in graph.get("nodes", []):
        rule = dot_escape(node["rule_hash"])
        label = (
            f'{dot_escape(node.get("rule_id", "?"))}: '
            f'{dot_escape(node.get("source_root", "?"))} -> '
            f'{dot_escape(node.get("target_root", "?"))}\\n{rule}'
        )
        lines.append(f'  "{rule}" [label="{label}"];')
    for edge in graph.get("edges", []):
        runtime = edge.get("evidence") == "runtime_observed"
        count = f' x{edge.get("observations", 0)}' if runtime else ""
        attrs = ",color=blue,penwidth=2" if runtime else ""
        lines.append(
            f'  "{dot_escape(edge["src_rule"])}" -> '
            f'"{dot_escape(edge["dst_rule"])}" '
            f'[label="{dot_escape(edge.get("target_path", "?"))}{count}"{attrs}];'
        )
    for index, item in enumerate(graph.get("unresolved_inputs", [])):
        lines.append(
            f'  input_{index} [shape=box,style=dashed,label="Input @ '
            f'{dot_escape(item["target_path"])}"];'
        )
        lines.append(
            f'  "{dot_escape(item["src_rule"])}" -> input_{index} '
            f'[style=dashed,label="{dot_escape(item["target_path"])}"];'
        )
    lines.append("}")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("graph", type=Path, help="base rule_graph.json")
    parser.add_argument("traces", type=Path, nargs="+", help="DSL trace files")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--dot", type=Path)
    args = parser.parse_args()

    try:
        graph = json.loads(args.graph.read_text(encoding="utf-8"))
        records = (
            record for trace in args.traces for record in read_records(trace)
        )
        merged = merge_graph(graph, records)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(
            json.dumps(merged, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        if args.dot:
            args.dot.parent.mkdir(parents=True, exist_ok=True)
            args.dot.write_text(render_dot(merged), encoding="utf-8")
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(error, file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
