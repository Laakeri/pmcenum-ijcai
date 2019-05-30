#pragma once

#include <vector>

#include "graph.hpp"

namespace triangulator {
namespace comb_enumerator {
std::vector<std::vector<int>> Pmcs(const Graph& graph);
std::vector<std::vector<int>> FindMinSeps(const Graph& graph, int ub);
} // namespace comb_enumerator
} // namespace triangulator
