#pragma once

#include <vector>

#include "enumerator.hpp"
#include "graph.hpp"
#include "sat_interface.hpp"
#include "cardinality_constraint_builder.hpp"

namespace triangulator {

class FixedSizeEnumerator : public Enumerator {
public:
  FixedSizeEnumerator(const Graph& graph, std::shared_ptr<SatInterface> solver, int minsep_encoding, int card_encoding);
  std::vector<std::vector<int>> AllPmcs(int k);
private:
  std::vector<Lit> cardinality_network_;
  int card_encoding_;
  TotalizerBuilder tb_;
};
} // namespace triangulator
