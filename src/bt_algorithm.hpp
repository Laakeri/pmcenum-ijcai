#pragma once

// Implementation of the BT-algorithm. Parameterized by the MergeCost and CliqueCost functions.

#include <vector>

#include "graph.hpp"

namespace triangulator {
class BtAlgorithm {
public:
  std::pair<int, std::vector<Edge> > Solve(const Graph& graph, const std::vector<std::vector<int> >& pmcs);
private:
  int MergeCost(int c1, int c2) const;
  int CliqueCost(const Graph& graph, const std::vector<int>& pmc, const std::vector<int>& parent_sep) const;
};
} // namespace triangulator
