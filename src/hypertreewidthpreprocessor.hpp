#pragma once

#include <vector>
#include <cassert>

#include "hypergraph.hpp"
#include "mcs.hpp"

namespace triangulator {

class HypertreewidthPreprocessor {
public:
  std::vector<HyperGraph> Preprocess(const HyperGraph& graph);
private:
  std::vector<HyperGraph> instances_;
  void Preprocess1(const HyperGraph& graph);
};
} // namespace triangulator
