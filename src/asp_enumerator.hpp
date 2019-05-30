#pragma once

#include <vector>

#include "graph.hpp"
#include "hypergraph.hpp"

namespace triangulator {
namespace asp_enumerator {
std::vector<std::vector<int>> TreewidthPmcs(const Graph& graph, int size);
std::vector<std::vector<int>> HypertreewidthPmcs(const HyperGraph& hypergraph, int size);
} // namespace asp_enumerator
} // namespace triangulator
