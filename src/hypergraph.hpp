#pragma once

#include <vector>
#include <iostream>

#include "graph.hpp"

namespace triangulator {

class HyperGraph {
public:
  explicit HyperGraph(int n);
  explicit HyperGraph(std::vector<std::vector<int>> edges);
  Graph PrimalGraph() const;
  const std::vector<std::vector<int>>& Edges() const;
  const std::vector<std::vector<int>> EdgesIn(const std::vector<int>& vs) const;
  void AddEdge(std::vector<int> edge);
  void Print(std::ostream& out) const;
  int n() const;
  int m() const;
private:
  Graph primal_;
  std::vector<std::vector<int>> edges_;
};


} // namespace triangulator
