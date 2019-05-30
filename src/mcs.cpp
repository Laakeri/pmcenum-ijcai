#include "mcs.hpp"

#include <vector>
#include <cassert>

#include "utils.hpp"
#include "graph.hpp"

namespace triangulator {
namespace mcs {

std::vector<int> Mcs(const Graph& graph) {
  std::vector<int> order(graph.n());
  static std::vector<int> label;
  static std::vector<char> rm;
  static std::vector<std::vector<int> > labels;
  utils::InitZero(label, graph.n());
  utils::InitZero(rm, graph.n());
  if (labels.size() < graph.n()) labels.resize(graph.n());
  for (int i = 0; i < graph.n(); i++) labels[i].clear();
  for (int i = 0; i < graph.n(); i++) labels[0].push_back(i);
  int max_label = 0;
  for (int it = graph.n() - 1; it >= 0; it--) {
    if (labels[max_label].size() == 0) {
      max_label--;
      it++;
      continue;
    }
    int x = labels[max_label].back();
    labels[max_label].pop_back();
    if (rm[x]) {
      it++;
      continue;
    }
    order[it] = x;
    for (int nx : graph.Neighbors(x)) {
      if (!rm[nx]) {
        label[nx]++;
        labels[label[nx]].push_back(nx);
        max_label = std::max(max_label, label[nx]);
      }
    }
    rm[x] = true;
  }
  return order;
}

McsMOutput McsM(const Graph& graph) {
  // these vectors are static to prevent reallocating memory every time this function is called
  static std::vector<int> label;
  static std::vector<char> rm, rc;
  static std::vector<std::vector<int> > reach;
  utils::InitZero(label, graph.n());
  utils::InitZero(rm, graph.n());
  utils::InitZero(rc, graph.n());
  if (reach.size() < graph.n()) reach.resize(graph.n());
  for (int i = 0; i < graph.n(); i++) reach[i].clear();
  std::vector<Edge> fill;
  std::vector<int> order(graph.n());
  std::vector<char> is_maximal_point(graph.n());
  // TODO: maybe better variable names?
  int prev_label = -1;
  for (int it = graph.n() - 1; it >= 0; it--) {
    int x = 0;
    int max_label = 0;
    for (int i = 0; i < graph.n(); i++) {
      if (!rm[i] && label[i] >= max_label) {
        x = i;
        max_label = label[x];
      }
    }
    assert(rm[x] == 0 && label[x] < graph.n());
    order[it] = x;
    is_maximal_point[it] = (label[x] <= prev_label);
    prev_label = label[x];
    for (int i = 0; i < graph.n(); i++) {
      rc[i] = false;
    }
    rc[x] = true;
    rm[x] = true;
    for (int y : graph.Neighbors(x)) {
      if (!rm[y]) {
        rc[y] = true;
        reach[label[y]].push_back(y);
      }
    }
    for (int i = 0; i < graph.n(); i++) {
      while (!reach[i].empty()) {
        int y = reach[i].back();
        reach[i].pop_back();
        for (int z : graph.Neighbors(y)) {
          if (!rm[z] && !rc[z]) {
            rc[z] = true;
            if (label[z] > i) {
              reach[label[z]].push_back(z);
              label[z]++;
              fill.push_back({x, z});
            } else {
              reach[i].push_back(z);
            }
          }
        }
      }
    }
    for (int y : graph.Neighbors(x)) {
      if (!rm[y]) label[y]++;
    }
  }
  return McsMOutput({fill, order, is_maximal_point});
}

std::vector<Graph> Atoms(const Graph& graph, const McsMOutput& mcs_m_output) {
  Graph filled_graph(graph);
  filled_graph.AddEdges(mcs_m_output.fill_edges);
  // Use static to not allocate memory every time this is called
  static std::vector<char> rm, block;
  utils::InitZero(rm, graph.n());
  utils::InitZero(block, graph.n());
  std::vector<Graph> atoms;
  for (int it = 0; it < graph.n(); it++) {
    int x = mcs_m_output.elimination_order[it];
    if (mcs_m_output.is_maximal_clique_point[it]) {
      std::vector<int> cand_clique;
      for (int nx : filled_graph.Neighbors(x)) {
        if (!rm[nx]) cand_clique.push_back(nx);
      }
      if (graph.IsClique(cand_clique)) {
        for (int y : cand_clique) block[y] = 1;
        std::vector<int> component = graph.FindComponentAndMark(x, block);
        for (int y : cand_clique) block[y] = 0;
        component.insert(component.end(), cand_clique.begin(), cand_clique.end());
        atoms.push_back(Graph(graph.EdgesIn(component)));
      }
    }
    rm[x] = true;
  }
  int found = 0; // TODO
  for (int x = 0; x < graph.n(); x++) {
    if (!block[x]) {
      std::vector<int> component = graph.FindComponentAndMark(x, block);
      atoms.push_back(Graph(graph.EdgesIn(component)));
      found++;
    }
  }
  assert(found == 1); // TODO
  return atoms;
}

int Treewidth(const Graph& graph) {
  if (graph.m() == 0) return 0;
  std::vector<int> order = Mcs(graph);
  std::vector<int> inv_order = utils::PermInverse(order);
  int treewidth = 0;
  for (int i = 0; i < graph.n(); i++) {
    int x = order[i];
    int nb = 0;
    std::vector<int> clq;
    for (int nx : graph.Neighbors(x)) {
      if (inv_order[nx] > i) {
        nb++;
        clq.push_back(nx);
      }
    }
    assert(graph.IsClique(clq));
    treewidth = std::max(treewidth, nb);
  }
  return treewidth;
}

} // namespace mcs
} // namespace triangulator
