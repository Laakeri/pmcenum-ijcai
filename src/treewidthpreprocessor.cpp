#include "treewidthpreprocessor.hpp"

#include <vector>
#include <cassert>
#include <queue>

#include "graph.hpp"
#include "mcs.hpp"

namespace triangulator {
namespace {

std::vector<Edge> GreedyDegree2(Graph& graph) {
  std::queue<int> d2;
  for (int i = 0; i < graph.n(); i++) {
    assert(graph.Neighbors(i).size() > 1);
    if (graph.Neighbors(i).size() == 2) {
      d2.push(i);
    }
  }
  std::vector<Edge> fill;
  while (!d2.empty()) {
    int x = d2.front();
    d2.pop();
    if (graph.Neighbors(x).size() == 1) {
      graph.RemoveEdge(x, graph.Neighbors(x)[0]);
    } else if (graph.Neighbors(x).size() == 2) {
      int nb1 = graph.Neighbors(x)[0];
      int nb2 = graph.Neighbors(x)[1];
      graph.RemoveEdge(x, nb1);
      graph.RemoveEdge(x, nb2);
      if (!graph.HasEdge(nb1, nb2)) {
        graph.AddEdge(nb1, nb2);
        fill.push_back({nb1, nb2});
      }
      if (graph.Neighbors(nb1).size() <= 2) {
        d2.push(nb1);
      }
      if (graph.Neighbors(nb2).size() <= 2) {
        d2.push(nb2);
      }
    } else {
      assert(graph.Neighbors(x).size() == 0);
    }
  }
  return fill;
}

Edge NeighborClique(Graph& graph) {
  for (int x = 0; x < graph.n(); x++) {
    assert(graph.Neighbors(x).size() > 2);
    Edge fo = {-1, -1};
    bool fail = false;
    for (int i = 0; i < (int)graph.Neighbors(x).size(); i++) {
      for (int ii = i + 1; ii < (int)graph.Neighbors(x).size(); ii++) {
        int u = graph.Neighbors(x)[i];
        int v = graph.Neighbors(x)[ii];
        if (!graph.HasEdge(u, v)) {
          if (fo.first == -1) {
            fo = {u, v};
          } else {
            fail = true;
            break;
          }
        }
      }
      if (fail) break;
    }
    if (!fail) {
      assert(fo.first >= 0 && fo.second >= 0 && fo.first < graph.n() && fo.second < graph.n());
      assert(!graph.HasEdge(fo));
      graph.AddEdge(fo);
      return fo;
    }
  }
  return {-1, -1};
}
} // namespace

TreewidthPreprocessor::TreewidthPreprocessor(const Graph& graph) : orig_graph_(graph) { }

std::vector<TreewidthInstance> TreewidthPreprocessor::Preprocess(bool pp) {
  lower_bound_ = 0;
  instances_.clear();
  fill_edges_.clear();
  if (pp) {
    Preprocess1(orig_graph_);
  } else {
    TreewidthInstance instance{orig_graph_, (int)1e9, {}};
    instances_.push_back(instance);
  }
  std::vector<TreewidthInstance> ret_instances;
  for (auto& t_instance : instances_) {
    if (lower_bound_ >= t_instance.upper_bound) {
      std::vector<Edge> fill = t_instance.graph.MapBack(t_instance.upper_bound_fill);
      fill_edges_.insert(fill_edges_.end(), fill.begin(), fill.end());
    } else {
      ret_instances.push_back(t_instance);
    }
  }
  instances_ = ret_instances;
  return instances_;
}

void TreewidthPreprocessor::Preprocess1(Graph graph) {
  mcs::McsMOutput minimal_triangulation = mcs::McsM(graph);
  auto fill = minimal_triangulation.fill_edges;
  Graph filled = graph;
  filled.AddEdges(fill);
  if (fill.size() <= 1) {
    lower_bound_ = std::max(lower_bound_, mcs::Treewidth(filled));
    fill = graph.MapBack(fill);
    fill_edges_.insert(fill_edges_.end(), fill.begin(), fill.end());
    return;
  }
  std::vector<Graph> atoms = mcs::Atoms(graph, minimal_triangulation);
  if (atoms.size() == 1) {
    int min_degree = atoms[0].MinDegree();
    assert(min_degree >= 2);
    lower_bound_ = std::max(lower_bound_, min_degree);
    atoms[0].InheritMap(graph);
    TreewidthInstance instance{atoms[0], mcs::Treewidth(filled), fill};
    Preprocess2(instance);
  } else {
    for (Graph& atom : atoms) {
      atom.InheritMap(graph);
      Preprocess1(atom);
    }
  }
}

void TreewidthPreprocessor::Preprocess2(TreewidthInstance instance) {
  auto fill_d2 = GreedyDegree2(instance.graph);
  if (fill_d2.size() > 0) {
    fill_d2 = instance.graph.MapBack(fill_d2);
    fill_edges_.insert(fill_edges_.end(), fill_d2.begin(), fill_d2.end());
    Preprocess1(instance.graph);
    return;
  }
  auto nb_c_fill = NeighborClique(instance.graph);
  if (nb_c_fill.first != -1) {
    fill_edges_.push_back(instance.graph.MapBack(nb_c_fill));
    Preprocess1(instance.graph);
    return;
  }
  instances_.push_back(instance);
}

TreewidthSolution TreewidthPreprocessor::MapBack(const std::vector<TreewidthSolution>& solutions) const {
  assert(solutions.size() == instances_.size());
  int is = instances_.size();
  TreewidthSolution solution;
  solution.treewidth = lower_bound_;
  solution.fill_edges = fill_edges_;
  for (int i = 0; i < is; i++) {
    solution.treewidth = std::max(solution.treewidth, solutions[i].treewidth);
    auto this_fill = instances_[i].graph.MapBack(solutions[i].fill_edges);
    solution.fill_edges.insert(solution.fill_edges.end(), this_fill.begin(), this_fill.end());
  }
  for (auto& edge : solution.fill_edges) {
    if (edge.first > edge.second) std::swap(edge.first, edge.second);
  }
  std::sort(solution.fill_edges.begin(), solution.fill_edges.end());
  for (int i = 1; i < solution.fill_edges.size(); i++) {
    assert(solution.fill_edges[i] != solution.fill_edges[i - 1]);
  }
  Graph final_fill = orig_graph_;
  for (auto edge : solution.fill_edges) {
    assert(edge.first >= 0 && edge.first < final_fill.n() && edge.second >= 0 && edge.second < final_fill.n());
    assert(!final_fill.HasEdge(edge));
    final_fill.AddEdge(edge);
  }
  assert(solution.treewidth == mcs::Treewidth(final_fill));
  return solution;
}
} // namespace triangulator