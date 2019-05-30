#include <vector>

#include "graph.hpp"
#include "hypergraph.hpp"

namespace triangulator {

int TreewidthSat(const Graph& graph, int minsep_enconding, int card_encoding, const std::string& solver, bool pp);
int TreewidthAsp(const Graph& graph, bool pp);
int TreewidthComb(const Graph& graph, bool pp);

int HypertreewidthSat(const HyperGraph& hypergraph, int minsep_enconding, int card_encoding, const std::string& solver);
int HypertreewidthAsp(const HyperGraph& hypergraph);
int HypertreewidthComb(const HyperGraph& hypergraph);

int CountMinseps(const Graph& graph, int ub);

} // namespace triangulator