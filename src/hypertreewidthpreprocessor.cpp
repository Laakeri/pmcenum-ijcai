#include "hypertreewidthpreprocessor.hpp"

#include <vector>
#include <cassert>

#include "graph.hpp"
#include "hypergraph.hpp"
#include "mcs.hpp"

namespace triangulator {
namespace {
const int inf = 1e9;
} // namespace

std::vector<HyperGraph> HypertreewidthPreprocessor::Preprocess(const HyperGraph& hypergraph) {
  instances_.clear();
  Preprocess1(hypergraph);
  return instances_;
}

void HypertreewidthPreprocessor::Preprocess1(const HyperGraph& hypergraph) {
  if (hypergraph.n() == 0) return;
  mcs::McsMOutput minimal_triangulation = mcs::McsM(hypergraph.PrimalGraph());
  std::vector<Graph> atoms = mcs::Atoms(hypergraph.PrimalGraph(), minimal_triangulation);
  if (atoms.size() == 1) {
    if (hypergraph.m() > 1) instances_.push_back(hypergraph);
  } else {
    for (Graph& atom : atoms) {
      std::vector<int> vss(atom.n());
      for (int i = 0; i < atom.n(); i++) {
        vss[i] = atom.MapBack(i);
      }
      HyperGraph hyper_atom(hypergraph.EdgesIn(vss));
      Preprocess1(hyper_atom);
    }
  }
}
} // namespace triangulator