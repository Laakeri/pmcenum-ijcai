#pragma once

#include <vector>
#include <cassert>

#include "graph.hpp"
#include "mcs.hpp"

namespace triangulator {

struct TreewidthInstance {
  Graph graph;
  int upper_bound;
  std::vector<Edge> upper_bound_fill;
};

struct TreewidthSolution {
  std::vector<Edge> fill_edges;
  int treewidth;
};

class TreewidthPreprocessor {
public:
  TreewidthPreprocessor(const Graph& graph);
  std::vector<TreewidthInstance> Preprocess(bool pp);
  TreewidthSolution MapBack(const std::vector<TreewidthSolution>& solutions) const;
private:
  std::vector<TreewidthInstance> instances_;
  std::vector<Edge> fill_edges_;
  int lower_bound_;
  const Graph orig_graph_;
  void Preprocess1(Graph graph);
  void Preprocess2(TreewidthInstance instance);
};
} // namespace triangulator
