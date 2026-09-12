"""Optional PyTorch encoders and experimental complete-policy readout."""

import math
import json
import torch
from torch import nn
from torch.nn.utils.rnn import pack_padded_sequence, pad_sequence


class SharedSequenceEncoder(nn.Module):
    """Ordered IR/metadata tokens plus numeric values and explicit availability."""

    def __init__(self, vocabulary_size, width=32):
        super().__init__()
        if type(vocabulary_size) is not int or vocabulary_size < 2 or type(width) is not int or width < 1:
            raise ValueError('invalid encoder dimensions')
        self.tokens = nn.Embedding(vocabulary_size, width, padding_idx=0)
        self.numbers = nn.Linear(2, width, bias=False)
        self.sequence = nn.GRU(width, width, batch_first=True)

    def forward(self, sequences, token_contexts=None):
        if token_contexts is None and len(sequences) > 1:
            # Deterministic shared f_theta(x): encode equal immutable inputs once
            # in THIS forward, then gather every occurrence back. Autograd sums
            # all occurrence gradients; no edge/count is removed or cached across
            # optimizer steps. Per-token differentiable contexts must stay separate.
            unique, lookup, indices = [], {}, []
            for sequence in sequences:
                key = json.dumps(sequence, sort_keys=True, allow_nan=False)
                if key not in lookup:
                    lookup[key] = len(unique)
                    unique.append(sequence)
                indices.append(lookup[key])
            if len(unique) < len(sequences):
                encoded = self._encode(unique)
                return encoded.index_select(0, torch.tensor(indices, device=encoded.device))
        return self._encode(sequences, token_contexts)

    def _encode(self, sequences, token_contexts=None):
        if token_contexts is not None and len(token_contexts) != len(sequences):
            raise ValueError('one token context per sequence is required')
        if not sequences:
            return self.tokens.weight.new_zeros((0, self.tokens.embedding_dim))
        device, dtype = self.tokens.weight.device, self.tokens.weight.dtype
        ids, values, lengths = [], [], []
        for sequence in sequences:
            tokens, numbers, known = (sequence[k] for k in ('tokens', 'numbers', 'known_number'))
            if (not tokens or len(tokens) != len(numbers) or len(tokens) != len(known)
                    or any(type(t) is not int or not 0 < t < self.tokens.num_embeddings for t in tokens)
                    or any(type(mask) is not bool for mask in known)):
                raise ValueError('invalid encoded sequence')
            token_ids = torch.tensor(tokens, dtype=torch.long, device=device)
            numeric = torch.tensor(list(zip(numbers, known)), dtype=dtype, device=device)
            if not torch.isfinite(numeric).all() or any(value != 0 for value, mask in zip(numbers, known) if not mask):
                raise ValueError('numeric input must be finite with zero storage for missing values')
            ids.append(token_ids)
            values.append(numeric)
            lengths.append(len(tokens))
        embedded = self.tokens(pad_sequence(ids, batch_first=True)) + self.numbers(pad_sequence(values, batch_first=True))
        if token_contexts is not None:
            for context, length in zip(token_contexts, lengths):
                if (not isinstance(context, torch.Tensor) or context.shape != (length, self.tokens.embedding_dim)
                        or context.device != device or context.dtype != dtype or not torch.isfinite(context).all()):
                    raise ValueError('invalid token context')
            embedded = embedded + pad_sequence(token_contexts, batch_first=True)
        packed = pack_padded_sequence(embedded, lengths, batch_first=True, enforce_sorted=False)
        _, hidden = self.sequence(packed)
        # A direct, padding-free path for early numeric fields: final GRU state alone
        # lost catalog row-count changes at float32 precision on real long sequences.
        valid = torch.arange(embedded.shape[1], device=device)[None, :] < torch.tensor(lengths, device=device)[:, None]
        pooled = (embedded * valid.unsqueeze(-1)).sum(1) / embedded.new_tensor(lengths).unsqueeze(-1)
        return hidden[0] + pooled


def query_context_embedding(encoder, features):
    """Inject pre-run relation metadata at bound SQL references, not derived-input cardinalities."""
    binding, sequences = features['query_binding'], features['sequences']
    if (binding.get('complete') is not True or len(sequences['query']) != 1
            or 'source_references' not in binding):
        raise ValueError('require one completely bound query')
    query, sources = sequences['query'][0], binding['sources']
    relations = encoder(sequences['relation'])
    context = relations.new_zeros((len(query['tokens']), encoder.tokens.embedding_dim))
    positions, rows, seen = [], [], set()
    for reference in binding['source_references']:
        token, source = reference['token'], reference['source']
        if (type(token) is not int or not 0 <= token < len(query['tokens']) or token in seen
                or type(source) is not int or not 0 <= source < len(sources)
                or sources[source]['source'] != source or query['known_number'][token] is not True
                or query['numbers'][token] != source):
            raise ValueError('invalid query source reference')
        seen.add(token)
        record = sources[source]
        if record['kind'] == 'relation':
            relation = record['relation']
            if type(relation) is not int or not 0 <= relation < len(relations):
                raise ValueError('query relation missing from encoded catalog')
            positions.append(token)
            rows.append(relations[relation])
        elif record['kind'] != 'derived':
            raise ValueError('unsupported query source kind')
    if positions:
        context = context.index_copy(0, torch.tensor(positions, device=context.device), torch.stack(rows))
    return encoder([query], [context])


class RuleAttemptEncoder(nn.Module):
    """Shared rule × local Memo context fusion; not a trained value or policy predictor."""

    def __init__(self, vocabulary_size, width=32):
        super().__init__()
        self.encoder = SharedSequenceEncoder(vocabulary_size, width)
        self.fusion = nn.Linear(3 * width + 1, width)

    def forward(self, rules, contexts, queries=None, query_embeddings=None):
        if len(rules) != len(contexts) or (queries is not None and len(queries) != len(rules)):
            raise ValueError('one rule and local context per attempt are required')
        rule, local = self.encoder(rules), self.encoder(contexts)
        if query_embeddings is not None:
            if (queries is not None or not isinstance(query_embeddings, torch.Tensor)
                    or query_embeddings.shape != rule.shape or query_embeddings.device != rule.device
                    or query_embeddings.dtype != rule.dtype or not torch.isfinite(query_embeddings).all()):
                raise ValueError('invalid or conflicting query embeddings')
            query = query_embeddings
        else:
            query = torch.zeros_like(rule) if queries is None else self.encoder(queries)
        available = rule.new_full((len(rules), 1), float(queries is not None or query_embeddings is not None))
        return torch.tanh(self.fusion(torch.cat((rule, local, query, available), dim=1)))


class DirectedRuleAggregation(nn.Module):
    """One synchronous step, separate predecessor/successor messages and position edges."""

    def __init__(self, width, rooted=False):
        super().__init__()
        self.rooted = rooted
        self.predecessor = nn.Linear((4 if rooted else 2) * width, width)
        self.successor = nn.Linear((4 if rooted else 2) * width, width)
        self.update = nn.Linear(3 * width + 2, width)

    def forward(self, nodes, edges, positions, roots=None):
        width = self.predecessor.out_features
        if (nodes.ndim != 2 or nodes.shape[1] != width or not torch.isfinite(nodes).all()
                or positions.shape != (len(edges), width) or positions.dtype != nodes.dtype
                or positions.device != nodes.device or not torch.isfinite(positions).all()
                or any(len(e) != 2 or any(type(i) is not int or not 0 <= i < len(nodes) for i in e) for e in edges)):
            raise ValueError('invalid directed graph tensors or endpoints')
        if self.rooted:
            if (not isinstance(roots, torch.Tensor) or roots.shape != (len(edges), 2, width)
                    or roots.dtype != nodes.dtype or roots.device != nodes.device or not torch.isfinite(roots).all()):
                raise ValueError('require source-target and destination-source root embeddings')
        elif roots is not None:
            raise ValueError('root features require a rooted aggregation layer')
        incoming, outgoing = torch.zeros_like(nodes), torch.zeros_like(nodes)
        indegree = nodes.new_zeros((len(nodes), 1))
        outdegree = torch.zeros_like(indegree)
        if edges:
            source, target = torch.tensor(edges, device=nodes.device, dtype=torch.long).T
            incoming_features = (nodes[source], roots[:, 0], roots[:, 1], positions) if self.rooted else (nodes[source], positions)
            outgoing_features = (nodes[target], roots[:, 1], roots[:, 0], positions) if self.rooted else (nodes[target], positions)
            incoming = incoming.index_add(0, target, torch.tanh(self.predecessor(
                torch.cat(incoming_features, 1))))
            outgoing = outgoing.index_add(0, source, torch.tanh(self.successor(
                torch.cat(outgoing_features, 1))))
            ones = nodes.new_ones((len(edges), 1))
            indegree.index_add_(0, target, ones)
            outdegree.index_add_(0, source, ones)
        # Mean plus degree retains multiplicity of position-distinct edges.
        context = torch.cat((nodes, incoming / indegree.clamp_min(1), outgoing / outdegree.clamp_min(1),
                             indegree.log1p(), outdegree.log1p()), 1)
        return nodes + torch.tanh(self.update(context))


class WholePolicyPredictor(nn.Module):
    """Shared set/static-graph models, predicting log1p milliseconds for planning/execution.

    Only prospective rule/policy/query/catalog channels are read. Current Memo,
    historical edges/counts, and response labels are intentionally not inputs.
    """

    def __init__(self, vocabulary_size, width=32, graph_mode='none'):
        super().__init__()
        if graph_mode not in ('none', 'static', 'self'):
            raise ValueError('unsupported graph ablation')
        self.graph_mode = graph_mode
        self.encoder = SharedSequenceEncoder(vocabulary_size, width)
        self.rule_context = nn.Sequential(nn.Linear(3 * width, width), nn.Tanh())
        self.readout = nn.Sequential(nn.Linear(2 * width + 1, width), nn.Tanh(), nn.Linear(width, 2))
        # Registered after baseline parameters, preserving their initialization.
        if graph_mode != 'none':
            self.messages = DirectedRuleAggregation(width)

    def forward(self, features):
        sequences, loaded = features['sequences'], features['loaded_rules']
        if (len(loaded) != len(sequences['rule']) or len(loaded) != len(sequences['policy'])
                or any(type(flag) is not bool for flag in loaded)):
            raise ValueError('invalid loaded rule membership')
        query = query_context_embedding(self.encoder, features)
        # Not-loaded graph nodes cannot influence this complete candidate policy.
        rules = self.encoder([s for s, keep in zip(sequences['rule'], loaded) if keep])
        policies = self.encoder([s for s, keep in zip(sequences['policy'], loaded) if keep])
        nodes = self.rule_context(torch.cat((rules, policies, query.expand(len(rules), -1)), dim=1))
        if self.graph_mode != 'none':
            edges = features['rule_edges']
            if (features.get('rule_graph_scope') != 'native_static_template'
                    or len(edges) != len(sequences['edge'])
                    or any(len(e) != 2 or any(type(i) is not int or not 0 <= i < len(loaded) for i in e) for e in edges)):
                raise ValueError('require verified native static edges')
            local = {i: j for j, i in enumerate(i for i, flag in enumerate(loaded) if flag)}
            selected = [i for i, (s, t) in enumerate(edges) if loaded[s] and loaded[t]] if self.graph_mode == 'static' else []
            positions = self.encoder([sequences['edge'][i] for i in selected])
            nodes = self.messages(nodes, [[local[j] for j in edges[i]] for i in selected], positions)
        # Sum saturated the downstream tanh for the real 103-rule catalog. Mean
        # plus count retains the sum information without its unbounded scale.
        pooled = nodes.sum(0, keepdim=True) / max(1, len(nodes))
        count = query.new_tensor([[math.log1p(len(nodes))]])
        return self.readout(torch.cat((query, pooled, count), dim=1))[0]
