#pragma once

#include <vector>

#include "enumerator.hpp"
#include "hypergraph.hpp"
#include "sat_interface.hpp"
#include "cardinality_constraint_builder.hpp"

namespace triangulator {

class FixedSizeHyperEnumerator : public Enumerator {
public:
  FixedSizeHyperEnumerator(const HyperGraph& graph, std::shared_ptr<SatInterface> solver, int minsep_encoding, int card_encoding);
  std::vector<std::vector<int>> AllPmcs(int k);
private:
  std::vector<Lit> cardinality_network_;
  int card_encoding_;
  TotalizerBuilder tb_;
};
} // namespace triangulator
