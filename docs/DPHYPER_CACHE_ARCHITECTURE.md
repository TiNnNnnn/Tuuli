# DPHyper Cache Architecture

> 更新时间：2026-08-27
> 适用分支：`dphyper-cascades`

## 1. 结论

DPHyper 简化阶段目前使用三层复用机制。只有第一层是 Memo 自身的统计缓存；另外两层用于避免重复构造
DPHyper 临时代价候选，但其有效性仍由 Memo 中的统计状态约束。

```text
Memo CGroup::Pstats()
  │  持久化逻辑等价组的统计信息
  ▼
单 region CDPHyperJoinCostCache
  │  NodeSet -> expression / rows / accumulated cost
  ▼
查询级 CEngine exact-stats cache
     exact expression -> derived expression with statistics
     validity guard: every Memo leaf still exposes the same Pstats object
```

DPHyper 的临时代价表达式不会插入 Memo。它们只用于给 Horn 风格 graph simplifier 排序，因此把所有临时
子集直接变成 Memo Group 会污染正常的 Cascades 搜索空间。查询级 cache 放在 `CEngine`，生命周期与一次
优化一致，并通过 Memo 叶子统计身份判断条目是否仍然有效。

## 2. 三层 Cache 的职责

### 2.1 Memo Group Statistics

`CGroup::Pstats()` 是 ORCA 的等价组统计缓存。多个 GroupExpression 属于同一个逻辑等价组时共享该统计。

统计相关 xform 只有真正产生 alternative 后才触发 `FResetStats()`。无输出的 xform 不再清空已有统计，
避免后续 GroupExpression 重复派生相同 Group 的统计。

### 2.2 Region-local Join Cost Cache

`CDPHyperJoinCostCache` 在一次 `CJobJoinEnumeration::FEnumerateRegion()` 中存活，以 region 内的节点子集
`CBitSet` 为键，保存：

- 表示该子集的临时代价表达式；
- 输出行数；
- 当前发现的最小累计 cost。

节点编号只在当前 region 内有意义，因此该 cache 不能直接跨 region 使用。它与 Horn 的 `NodeMap ->
stats/cost` 模型对应，并且不会减少 DPHyper 产生的 CSG-CMP pair。

### 2.3 Query-local Exact Statistics Cache

不同 region 可能再次构造完全相同的临时代价表达式。`CEngine` 维护查询级精确 cache，以表达式结构 hash
分桶，再执行完整结构比较：

- 普通 operator 和 scalar predicate 使用 operator 的精确 `Matches()`；
- Memo 叶子必须引用同一个 `CGroup`；
- hash 只用于分桶，不能替代完整相等判断；
- 命中时返回带有已派生 statistics 的表达式，而不只返回一个行数标量。

保留完整表达式很重要：父 Join 的统计派生需要访问孩子的统计对象，仅缓存 `rows` 仍会使父表达式递归
重新派生孩子统计。

## 3. 失效与正确性约束

查询级条目在复用前递归检查每个关系叶子：

```text
cached_leaf.Pstats() == cached_leaf.Pgexpr()->Pgroup()->Pstats()
```

缓存表达式持有旧 statistics 的引用。因此 Group stats 被重置后，即使内存分配器后续复用地址，也不会
出现旧地址与新对象误判相同的 ABA 问题。只要任意关系叶子的统计对象发生变化，该条目立即视为失效。

当前实现刻意不做以下放宽：

- 不把只有 hash 相同的表达式视为相同；
- 不按 region-local NodeSet 跨 region 复用；
- 不把 duplicate Group 的不同原始 id 直接视为精确叶子相同；
- 不对 InnerJoin 交换输入后做近似命中；
- 不按 SQL、rule id、表名或列名增加特例。

这些限制使 cache 只复用已经实际派生过的相同逻辑表达式，不改变 graph simplifier 的 cost 序列、选择的
simplification steps 或最终枚举空间。

## 4. 重复预算探测

Horn 风格简化器先指数扩大 simplification step 数量，再二分搜索满足 pair budget 的最小 step 数。
二分过程中，`upper` 只有在对应图已经完成一次不超预算的完整枚举后才会降低。因此二分收敛后重新应用
`upper` 只是在恢复已验证的图，不需要立即再次执行相同预算探测；调用者随后还会运行一次用于物化结果的
最终 DPHyper 枚举。

当前实现删除了这次重复 probe。它不改变 step 数和最终 pair，只消除一次无输出的完整枚举。

## 5. Profile 结果

样本为私有 WeTune corpus 中原先的一条 21-table 超时热点 SQL；规则正文和私有 corpus 不进入 Git。
下面数据来自同一 Release 构建、同一 pair budget 的单次本地 profile，时间会受机器负载影响。

| 指标 | 优化前 | 优化后 |
|---|---:|---:|
| DPHyper cost 请求 | 12,204 | 12,204 |
| region-local output 命中 | 10,736 | 10,736 |
| region-local 命中率 | 88.0% | 88.0% |
| query-local exact stats 命中 | 0 | 339 |
| Join enumeration job 时间 | 约 1.61 s | 约 1.42 s |
| peak Memo groups | 1,379 | 1,379 |
| peak duplicate groups | 20 | 20 |
| peak GroupExpressions | 4,311 | 4,311 |
| DSL binding attempts | 22,349 | 22,349 |
| DSL generated alternatives | 261 | 261 |

五个需要 graph simplification 的大 region 中，query-local cache 分别在后续 region 命中 106、165 和
68 次，共避免 339 次完整 stats 派生。predicate 和 Join 节点构造只占毫秒级，统计派生仍是剩余简化成本
中的主要部分。

## 6. 验证

- `CDPHyperGraphTest`：通过，包括 exhaustive/differential、budget、simplifier 和复杂 Join region 用例；
- `CDSLEngineTest`：通过；
- DSL off/on E2E：41/41 通过；
- 热点 SQL 的 WeTune 预期规则全部覆盖；
- DPHyper pair 数、simplification step、Memo 规模和 DSL 搜索计数保持不变。

## 7. 后续方向

下一步应继续 profile `CDPHyperGraphSimplifier::MakeStep()`、bitset 临时对象和预算 probe，而不是扩大 cache
的等价定义。任何新优化都必须保持相同的合法 pair、相同的最小预算约束和相同的 fallback 行为。
