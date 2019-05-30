#include "fixed_size_hyper_enumerator.hpp"

#include <vector>
#include <cassert>

#include "hypergraph.hpp"
#include "graph.hpp"
#include "sat_interface.hpp"
#include "utils.hpp"
#include "cardinality_constraint_builder.hpp"

namespace triangulator {

FixedSizeHyperEnumerator::FixedSizeHyperEnumerator(const HyperGraph& graph, std::shared_ptr<SatInterface> solver, int minsep_encoding, int card_encoding)
  : Enumerator(graph.PrimalGraph(), solver, minsep_encoding), card_encoding_(card_encoding), tb_(solver) {
  std::vector<Lit> edge_vars;
  std::vector<std::vector<int> > in_edge(graph.n());
  for (int i = 0; i < graph.m(); i++) {
    Lit nv = solver->NewVar();
    edge_vars.push_back(nv);
    for (int v : graph.Edges()[i]) {
      in_edge[v].push_back(i);
    }
  }
  for (int i = 0; i < graph.n(); i++) {
    std::vector<Lit> n_clause;
    n_clause.push_back(-XVars()[i]);
    for (int e : in_edge[i]) {
      n_clause.push_back(edge_vars[e]);
    }
    solver->AddClause(n_clause);
  }
  if (card_encoding == 0) {
    CardinalityNetworkBuilder ccb(solver);
    cardinality_network_ = ccb.EqualNetwork(edge_vars);
  } else if (card_encoding == 1) {
    cardinality_network_ = tb_.Init(edge_vars);
  } else {
    assert(0);
  }
  assert(cardinality_network_.size() == (int)graph.m());
  for (Lit var : cardinality_network_) {
    solver->FreezeVar(var);
  }
}

std::vector<std::vector<int>> FixedSizeHyperEnumerator::AllPmcs(int k) {
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
