"""Ordered recursive DSL trees, bound constraint factors, and rooted multi-round messages."""

import math
import torch
from torch import nn

from rule_policy_model import DirectedRuleAggregation, WholePolicyPredictor, query_context_embedding


def context_summary(values, destinations, weights, rule_count, variance=False):
    """Attempt-weighted empirical moments of encoded contexts, not new graph nodes.

    Central moments avoid subtracting two nearly equal second moments. This is
    a finite summary, not an injective encoding of the context distribution.
    """
    counts = values.new_zeros((rule_count, 1)).index_add(0, destinations, weights)
    mean = values.new_zeros((rule_count, values.shape[1])).index_add(0, destinations, weights * values)
    mean = mean / counts.clamp_min(1)
    summary = [mean]
    if variance:
        residual = values - mean[destinations]
        moment = torch.zeros_like(mean).index_add(0, destinations, weights * residual.square())
        summary.append(moment / counts.clamp_min(1))
    return torch.cat((*summary, torch.log1p(counts)), 1)


class RuleTreeEncoder(nn.Module):
    """Ordered, arbitrary-arity Tree-LSTM with constraint factors linked through symbols.

    A sibling GRU supplies the ordered child summary to the Tree-LSTM gates;
    children retain individual cell states and forget gates. No DSL subtree is
    serialized into a recurrent sentence. Local attribute lists remain ordered.
    """

    def __init__(self, width):
        super().__init__()
        self.child_order = nn.GRU(width, width, batch_first=True)
        self.iou = nn.Linear(2 * width, 3 * width)
        self.forget = nn.Linear(2 * width, width)
        self.slot = nn.Linear(1, width, bias=False)
        self.symbol_context = nn.Linear(2 * width + 1, width)
        self.constraint_context = nn.Linear(width, width, bias=False)
        self.rule_readout = nn.Linear(3 * width + 1, width)

    def tree_states(self, inputs, nodes):
        if len(inputs) != len(nodes) or not nodes:
            raise ValueError('one feature vector per tree node required')
        hidden, cells = [], []
        for index, (value, node) in enumerate(zip(inputs, nodes)):
            children = node['children']
            if any(type(child) is not int or not 0 <= child < index for child in children):
                raise ValueError('tree children must precede their parent')
            summary, child_cell = torch.zeros_like(value), torch.zeros_like(value)
            if children:
                child_hidden = torch.stack([hidden[c] for c in children])
                summary = self.child_order(child_hidden.unsqueeze(0))[1][0, 0]
                forget = torch.sigmoid(self.forget(torch.cat((value.expand(len(children), -1), child_hidden), 1)))
                child_cell = (forget * torch.stack([cells[c] for c in children])).sum(0)
            i, o, u = self.iou(torch.cat((value, summary))).chunk(3)
            cell = torch.sigmoid(i) * torch.tanh(u) + child_cell
            cells.append(cell)
            hidden.append(torch.sigmoid(o) * torch.tanh(cell))
        return torch.stack(hidden)

    def forward(self, structure, sequences, encoder):
        def features(channel):
            bounds = structure[channel + '_range']
            if (len(bounds) != 2 or any(type(i) is not int for i in bounds)
                    or not 0 <= bounds[0] <= bounds[1] <= len(sequences[channel])):
                raise ValueError('invalid rule feature range')
            return sequences[channel][bounds[0]:bounds[1]]

        node_features = encoder(features('rule_node'))
        symbol_features = encoder(features('rule_symbol'))
        constraint_sequences = features('rule_constraint')
        nodes, roots = structure['nodes'], structure['roots']
        occurrences, references = structure['symbol_occurrences'], structure['constraint_references']
        if (not nodes or len(nodes) != len(node_features) or len(occurrences) != len(symbol_features)
                or len(references) != len(constraint_sequences) or set(roots) != {'source', 'target'}
                or any(type(i) is not int or not 0 <= i < len(nodes) or nodes[i]['side'] != side
                       or nodes[i]['path'] != 'r' for side, i in roots.items())):
            raise ValueError('invalid rule tree/constraint layout')
        # The compiler supplies a forest, not a DAG. Validate ownership and links
        # before using them as tensor gather/scatter indices.
        parents = [0] * len(nodes)
        actual_occurrences = [[] for _ in occurrences]
        for index, node in enumerate(nodes):
            for position, child in enumerate(node['children']):
                if (type(child) is not int or not 0 <= child < index
                        or nodes[child]['side'] != node['side']
                        or nodes[child]['path'] != node['path'] + '/' + str(position)):
                    raise ValueError('invalid ordered tree edge')
                parents[child] += 1
            for slot, ref in enumerate(node['symbols']):
                if type(ref) is not int or not 0 <= ref < len(occurrences):
                    raise ValueError('invalid node symbol reference')
                actual_occurrences[ref].append([index, slot])
        if actual_occurrences != occurrences or any(n != (0 if i in roots.values() else 1) for i, n in enumerate(parents)):
            raise ValueError('inconsistent tree ownership or symbol incidence')
        initial = self.tree_states(node_features, nodes)
        symbols = []
        for value, sites in zip(symbol_features, occurrences):
            context = torch.zeros_like(value)
            if sites:
                slots = value.new_tensor([[math.log1p(slot)] for _, slot in sites])
                context = (initial[[node for node, _ in sites]] + self.slot(slots)).mean(0)
            symbols.append(torch.tanh(self.symbol_context(torch.cat((value, context,
                                      value.new_tensor([math.log1p(len(sites))]))))))
        contexts = []
        incidence = [[] for _ in symbols]
        for index, (sequence, args) in enumerate(zip(constraint_sequences, references)):
            context = initial.new_zeros((len(sequence['tokens']), initial.shape[1]))
            tokens, values = [], []
            for arg in args:
                token, ref = arg['token'], arg['symbol']
                if (type(token) is not int or not 0 <= token < len(sequence['tokens']) or token in tokens
                        or type(ref) is not int or not 0 <= ref < len(symbols)
                        or not sequence['known_number'][token] or sequence['numbers'][token] != ref):
                    raise ValueError('invalid bound constraint argument')
                tokens.append(token)
                values.append(symbols[ref])
                incidence[ref].append(index)
            if tokens:
                context = context.index_copy(0, torch.tensor(tokens, device=context.device), torch.stack(values))
            contexts.append(context)
        constraints = encoder(constraint_sequences, contexts)
        symbol_constraints = [constraints[indices].mean(0) if indices else initial.new_zeros(initial.shape[1])
                              for indices in incidence]
        local = []
        for value, node in zip(node_features, nodes):
            bound = torch.stack([symbol_constraints[r] for r in node['symbols']]).mean(0) if node['symbols'] else torch.zeros_like(value)
            local.append(value + self.constraint_context(bound))
        states = self.tree_states(torch.stack(local), nodes)
        constraint_summary = constraints.mean(0) if len(constraints) else torch.zeros_like(states[0])
        rule = torch.tanh(self.rule_readout(torch.cat((states[roots['source']], states[roots['target']],
                   constraint_summary, states.new_tensor([math.log1p(len(constraints))])))))
        return rule, states


class TreePolicyPredictor(WholePolicyPredictor):
    """Constraint-aware tree roots condition each directed message at every graph round."""

    def __init__(self, vocabulary_size, width=32, graph_mode='static', message_rounds=3, history=False,
                 history_mode='graph', history_pooling='mean'):
        if graph_mode not in ('none', 'static', 'self') or type(message_rounds) is not int or message_rounds < 1:
            raise ValueError('invalid tree graph configuration')
        super().__init__(vocabulary_size, width, 'none')
        self.graph_mode = graph_mode
        self.rule_trees = RuleTreeEncoder(width)
        self.rounds = nn.ModuleList([DirectedRuleAggregation(width, rooted=True)
                                    for _ in range(message_rounds if graph_mode != 'none' else 0)])
        self.use_history = history
        if history_mode not in ('none', 'context', 'graph'):
            raise ValueError('invalid history ablation')
        self.history_mode = history_mode
        if history_pooling not in ('mean', 'mean_variance'):
            raise ValueError('invalid historical context pooling')
        self.history_pooling = history_pooling
        if history:
            self.history_run = nn.Linear(2 * width + 1, width)
            self.history_attempt = nn.Linear(4 * width, width)
            self.history_update = nn.Linear((2 if history_pooling == 'mean_variance' else 1) * width + 1,
                                            width, bias=False)
            self.history_ports = nn.Linear(width, 2 * width)

    def history_inputs(self, features, query, local):
        """Historical actual trees with behavior query/catalog/policy, no current Memo."""
        if features.get('history_admission', {}).get('capture') != 'audited_history_frozen':
            raise ValueError('historical observations require admission')
        sequences = features['sequences']

        def take(channel, bounds):
            values = sequences[channel]
            if (len(bounds) != 2 or any(type(i) is not int for i in bounds)
                    or not 0 <= bounds[0] <= bounds[1] <= len(values)):
                raise ValueError('invalid history feature range')
            return values[bounds[0]:bounds[1]]

        runs, policies = [], []
        for run in features['history_runs']:
            context = query_context_embedding(self.encoder, {'query_binding': run['query_binding'], 'sequences': {
                'query': take('history_query', run['query_range']),
                'relation': take('history_relation', run['relation_range'])}})[0]
            policy = self.encoder(take('history_policy', run['policy_range']))
            membership = run['loaded_rules']
            if len(membership) != len(policy) or any(type(v) is not bool for v in membership):
                raise ValueError('invalid historical loaded policy membership')
            active = [i for i, loaded in enumerate(membership) if loaded]
            summary = policy[active].mean(0) if active else torch.zeros_like(context)
            runs.append(torch.tanh(self.history_run(torch.cat((context, summary,
                                    context.new_tensor([math.log1p(len(active))]))))))
            policies.append(policy)
        states = []
        for tree in features['history_trees']:
            if tree is None:
                states.append(None)
                continue
            nodes, root = tree['nodes'], tree['root']
            if type(root) is not int or not 0 <= root < len(nodes) or nodes[root]['path'] != 'r':
                raise ValueError('invalid historical tree root')
            ownership = [0] * len(nodes)
            for node in nodes:
                for slot, child in enumerate(node['children']):
                    if (type(child) is not int or not 0 <= child < len(nodes)
                            or nodes[child]['path'] != node['path'] + '/' + str(slot)):
                        raise ValueError('invalid historical ordered child')
                    ownership[child] += 1
            if any(n != (0 if i == root else 1) for i, n in enumerate(ownership)):
                raise ValueError('invalid historical tree ownership')
            states.append(self.rule_trees.tree_states(self.encoder(take('history_node', tree['node_range'])), nodes))
        attempts = self.encoder(sequences['history_attempt'])
        if len(attempts) != len(features['history_contexts']):
            raise ValueError('historical attempt feature mismatch')
        destinations, weights, inputs = [], [], []
        for i, item in enumerate(features['history_contexts']):
            rule, run, tree, count = (item[k] for k in ('rule', 'run', 'tree', 'count'))
            if type(count) is not int or count < 1:
                raise ValueError('invalid historical attempt count')
            if rule not in local:
                continue
            observed = states[tree]
            root = torch.zeros_like(query[0]) if observed is None else observed[features['history_trees'][tree]['root']]
            inputs.append(torch.cat((root, runs[run], policies[run][rule], attempts[i])))
            destinations.append(local[rule])
            weights.append([count])
        destinations = torch.tensor(destinations, dtype=torch.long, device=query.device)
        weights = query.new_tensor(weights).reshape(-1, 1)
        values = query.new_zeros((0, query.shape[1]))
        if inputs:
            # Same weighted sum, one batched projection/scatter instead of a
            # per-attempt autograd CopySlices chain. No observations are sampled.
            values = torch.tanh(self.history_attempt(torch.stack(inputs)))
        update = self.history_update(context_summary(values, destinations, weights, len(local),
                                     variance=self.history_pooling == 'mean_variance'))
        edges, positions, roots = [], [], []
        if len(sequences['history_edge']) != len(features['history_edges']):
            raise ValueError('historical edge feature mismatch')
        if self.history_mode != 'graph':
            return update, [], query.new_zeros((0, query.shape[1])), query.new_zeros((0, 2, query.shape[1]))
        encoded_edges = self.encoder(sequences['history_edge'])
        for i, edge in enumerate(features['history_edges']):
            if not all(rule in local for rule in edge['rules']):
                continue
            tree, root = edge['tree'], edge['root']
            if (states[tree] is None or type(root) is not int or not 0 <= root < len(states[tree])):
                raise ValueError('historical binding port is unobserved')
            edges.append([local[rule] for rule in edge['rules']])
            positions.append(encoded_edges[i] + runs[edge['run']])
            roots.append(states[tree][root])
        return update, edges, (torch.stack(positions) if positions else query.new_zeros((0, query.shape[1]))), (
            self.history_ports(torch.stack(roots)).reshape(-1, 2, query.shape[1]) if roots else query.new_zeros((0, 2, query.shape[1])))

    def forward(self, features):
        sequences, loaded, trees = features['sequences'], features['loaded_rules'], features['rule_trees']
        if (len(loaded) != len(trees) or len(loaded) != len(sequences['policy'])
                or any(type(flag) is not bool for flag in loaded)):
            raise ValueError('invalid loaded tree rule membership')
        query = query_context_embedding(self.encoder, features)
        active = [i for i, flag in enumerate(loaded) if flag]
        local = {index: position for position, index in enumerate(active)}
        encoded = [self.rule_trees(trees[i], sequences, self.encoder) for i in active]
        rules = torch.stack([r for r, _ in encoded]) if encoded else query.new_zeros((0, query.shape[1]))
        policies = self.encoder([sequences['policy'][i] for i in active])
        nodes = self.rule_context(torch.cat((rules, policies, query.expand(len(rules), -1)), 1))
        history = None
        if self.use_history and self.history_mode != 'none':
            history = self.history_inputs(features, query, local)
            nodes = nodes + history[0]
        elif not self.use_history and 'history_runs' in features:
            raise ValueError('model must explicitly enable historical inputs')
        if self.rounds:
            edges, endpoints = features['rule_edges'], features['edge_roots']
            if (features.get('rule_graph_scope') != 'native_static_template'
                    or len(edges) != len(sequences['edge']) or len(edges) != len(endpoints)
                    or any(len(e) != 2 or any(type(i) is not int or not 0 <= i < len(trees) for i in e) for e in edges)):
                raise ValueError('require verified root-bound static graph')
            selected = [i for i, (s, t) in enumerate(edges) if loaded[s] and loaded[t]] if self.graph_mode == 'static' else []
            pairs = []
            for i in selected:
                if len(endpoints[i]) != 2:
                    raise ValueError('require two dependency roots')
                pair = []
                for rule, root, side in zip(edges[i], endpoints[i], ('target', 'source')):
                    if (type(root) is not int or not 0 <= root < len(trees[rule]['nodes'])
                            or trees[rule]['nodes'][root]['side'] != side):
                        raise ValueError('dependency root has wrong side or index')
                    pair.append(encoded[local[rule]][1][root])
                pairs.append(torch.stack(pair))
            root_vectors = torch.stack(pairs) if pairs else query.new_zeros((0, 2, query.shape[1]))
            positions = self.encoder([sequences['edge'][i] for i in selected])
            links = [[local[r] for r in edges[i]] for i in selected]
            if history is not None and self.graph_mode == 'static':
                links += history[1]
                positions = torch.cat((positions, history[2]))
                root_vectors = torch.cat((root_vectors, history[3]))
            for layer in self.rounds:
                nodes = layer(nodes, links, positions, root_vectors)
        pooled = nodes.sum(0, keepdim=True) / max(1, len(nodes))
        return self.readout(torch.cat((query, pooled, query.new_tensor([[math.log1p(len(nodes))]])), 1))[0]
