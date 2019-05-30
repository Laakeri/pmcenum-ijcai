#include "fixed_size_enumerator.hpp"

#include <vector>
#include <cassert>

#include "graph.hpp"
#include "sat_interface.hpp"
#include "utils.hpp"
#include "cardinality_constraint_builder.hpp"

namespace triangulator {

FixedSizeEnumerator::FixedSizeEnumerator(const Graph& graph, std::shared_ptr<SatInterface> solver, int minsep_encoding, int card_encoding)
  : Enumerator(graph, solver, minsep_encoding), card_encoding_(card_encoding), tb_(solver) {
  if (card_encoding == 0) {
    CardinalityNetworkBuilder ccb(solver);
    cardinality_network_ = ccb.EqualNetwork(XVars());
  } else if (card_encoding == 1) {
    cardinality_network_ = tb_.Init(XVars());
  } else {
    assert(0);
  }
  assert(cardinality_network_.size() == (int)graph.n());
  for (Lit var : cardinality_network_) {
    solver->FreezeVar(var);
  }
}

std::vector<std::vector<int>> FixedSizeEnumerator::AllPmcs(int k) {
  if (card_encoding_ == 1) {
    tb_.BuildToSize(k+1);
  }
  std::vector<Lit> assumptions;
  for (int i = 0; i < (int)cardinality_network_.size(); i++) {
    if (i < k) {
      assumptions.push_back(cardinality_network_[i]);
    } else {
      assumptions.push_back(-cardinality_network_[i]);
    }
  }
  std::vector<std::vector<int>> pmcs;
  bool first_call = true;
  while (true) {
    auto pmc = GetPmc(assumptions, first_call);
    first_call = false;
    if (pmc.size() == 0) {
      break;
    } else {
      pmcs.push_back(pmc);
    }
  }
  return pmcs;
}
} // namespace triangulator
