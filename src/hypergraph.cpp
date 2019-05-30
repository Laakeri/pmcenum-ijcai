#include "hypergraph.hpp"

#include <vector>
#include <algorithm>
#include <iostream>

#include "utils.hpp"

namespace triangulator {
namespace {
std::vector<std::pair<int, int>> PrimalEdges(const std::vector<std::vector<int>>& edges) {
  std::vector<std::pair<int, int>> es;
  for (auto& e : edges) {
    for (int i = 0; i < e.size(); i++) {
      for (int ii = i + 1; ii < e.size(); ii++) {
        es.push_back({e[i], e[ii]});
      }
    }
  }
  return es;
}
} // namespace

HyperGraph::HyperGraph(int n) : primal_(n) { }

HyperGraph::HyperGraph(std::vector<std::vector<int>> edges) : primal_(PrimalEdges(edges)), edges_(edges) {
  for (auto& e : edges_) {
    for (int& v : e) {
      v = primal_.MapInto(v);
    }
    utils::SortAndDedup(e);
  }
}
Graph HyperGraph::PrimalGraph() const {
  return primal_;
}
const std::vector<std::vector<int> >& HyperGraph::Edges() const {
  return edges_;
}
const std::vector<std::vector<int>> HyperGraph::EdgesIn(const std::vector<int>& vs) const {
  static std::vector<char> is;
  utils::InitZero(is, primal_.n());
  for (int v : vs) {
    is[v] = true;
  }
  std::vector<std::vector<int>> edges;
  for (const auto& e : edges_) {
    std::vector<int> new_edge;
    for (int v : e) {
      if (is[v]) {
        new_edge.push_back(v);
      }
    }
    if ((int)new_edge.size() >= 2) edges.push_back(new_edge);
  }
  return edges;
}
void HyperGraph::AddEdge(std::vector<int> edge) {
  utils::SortAndDedup(edge);
  for (int i = 0; i < edge.size(); i++) {
    for (int ii = i + 1; ii < edge.size(); ii++) {
      primal_.AddEdge(edge[i], edge[ii]);
    }
  }
  edges_.push_back(edge);
}
void HyperGraph::Print(std::ostream& out) const {
  primal_.Print(out);
  out<<"hes: "<<edges_.size()<<std::endl;
  for (auto& e : edges_) {
    out<<"e:";
    for (int v : e) {
      out<<" "<<v;
    }
    out<<std::endl;
  }
}
int HyperGraph::n() const {
  return primal_.n();
}
int HyperGraph::m() const {
  return edges_.size();
}
} // namespace triangulator
