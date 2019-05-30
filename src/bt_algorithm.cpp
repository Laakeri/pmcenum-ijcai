#include "bt_algorithm.hpp"

#include <vector>
#include <map>
#include <cassert>
#include <queue>

#include "graph.hpp"
#include "id_set.hpp"

namespace triangulator {

int BtAlgorithm::MergeCost(int c1, int c2) const {
  return std::max(c1, c2);
}

int BtAlgorithm::CliqueCost(const Graph& graph, const std::vector<int>& pmc, const std::vector<int>& parent_sep) const {
  return pmc.size() - 1;
}

std::pair<int, std::vector<Edge>> BtAlgorithm::Solve(const Graph& graph, const std::vector<std::vector<int> >& pmcs) {
  std::vector<std::tuple<int, int, int> > triplets;
  IdSet<std::vector<int> > separators, components;
  std::vector<int> all_vertices(graph.n());
  for (int i = 0; i < graph.n(); i++) all_vertices[i] = i;
  int none_sep_id = separators.Insert({});
  int all_comp_id = components.Insert(all_vertices);
  for (int i = 0; i < pmcs.size(); i++) {
    const auto& pmc = pmcs[i];
    for (const auto& out_component : graph.Components(pmc)) {
      std::vector<int> separator = graph.Neighbors(out_component);
      assert(separator.size() < pmc.size());
      std::vector<char> in_separator(graph.n());
      for (int v : separator) in_separator[v] = true;
      std::vector<int> component;
      int found = 0;
      for (int v : pmc) {
        if (!in_separator[v]) {
          component = graph.FindComponentAndMark(v, in_separator);
          found++;
        }
      }
      assert(found == 1);
      std::sort(separator.begin(), separator.end());
      std::sort(component.begin(), component.end());
      int sep_id = separators.Insert(separator);
      int comp_id = components.Insert(component);
      triplets.push_back(std::make_tuple(i, sep_id, comp_id));
    }
    triplets.push_back(std::make_tuple(i, none_sep_id, all_comp_id));
  }
  auto cmp = [&](std::tuple<int, int, int> a, std::tuple<int, int, int> b) {
    int size_a = separators.Get(std::get<1>(a)).size() + components.Get(std::get<2>(a)).size();
    int size_b = separators.Get(std::get<1>(b)).size() + components.Get(std::get<2>(b)).size();
    return size_a < size_b;
  };
  std::sort(triplets.begin(), triplets.end(), cmp);
  std::map<std::pair<int, int>, int> dp, opt_choice;
  for (const auto& triplet : triplets) {
    const auto& pmc = pmcs[std::get<0>(triplet)];
    const auto& separator = separators.Get(std::get<1>(triplet));
    const auto& component = components.Get(std::get<2>(triplet));
    int cost  = CliqueCost(graph, pmc, separator);
    bool child_missing = false;
    std::vector<char> in_pmc(graph.n());
    for (int v : pmc) in_pmc[v] = true;
    for (int v : component) {
      if (!in_pmc[v]) {
        auto child_component = graph.FindComponentAndMark(v, in_pmc);
        auto child_separator = graph.Neighbors(child_component);
        std::sort(child_component.begin(), child_component.end());
        std::sort(child_separator.begin(), child_separator.end());
        std::pair<int, int> child_state = {separators.IdOf(child_separator), components.IdOf(child_component)};
        if (dp.count(child_state)) {
          cost = MergeCost(cost, dp[child_state]);
        } else {
          child_missing = true;
          break;
        }
      }
    }
    if (child_missing) continue;
    std::pair<int, int> this_state = {std::get<1>(triplet), std::get<2>(triplet)};
    if (!dp.count(this_state) || cost < dp[this_state]) {
      dp[this_state] = cost;
      opt_choice[this_state] = std::get<0>(triplet);
    }
  }
  if (!dp.count({none_sep_id, all_comp_id})) return {-1, {}};
  else {
    std::vector<Edge> fill_edges;
    std::queue<std::pair<int, int>> reconstruct;
    reconstruct.push({none_sep_id, all_comp_id});
    while (!reconstruct.empty()) {
      std::pair<int, int> state = reconstruct.front();
      reconstruct.pop();
      assert(dp.count(state));

      const auto& pmc = pmcs[opt_choice[state]];
      const auto& separator = separators.Get(state.first);
      const auto& component = components.Get(state.second);

      for (int i = 0; i < (int)pmc.size(); i++) {
        for (int ii = i+1; ii < (int)pmc.size(); ii++) {
          int u = pmc[i];
          int v = pmc[ii];
          if (!graph.HasEdge(u, v)) {
            if (!std::binary_search(separator.begin(), separator.end(), u) || !std::binary_search(separator.begin(), separator.end(), v)) {
              fill_edges.push_back({u, v});
            }
          }
        }
      }

      std::vector<char> in_pmc(graph.n());
      for (int v : pmc) in_pmc[v] = true;
      for (int v : component) {
        if (!in_pmc[v]) {
          auto child_component = graph.FindComponentAndMark(v, in_pmc);
          auto child_separator = graph.Neighbors(child_component);
          std::sort(child_component.begin(), child_component.end());
          std::sort(child_separator.begin(), child_separator.end());
          std::pair<int, int> child_state = {separators.IdOf(child_separator), components.IdOf(child_component)};
          assert(dp.count(child_state));
          reconstruct.push(child_state);
        }
      }
    }
    return {dp[{none_sep_id, all_comp_id}], fill_edges};
  }
}

} // namespace triangulator

