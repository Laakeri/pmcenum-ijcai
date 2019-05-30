#include "comb_enum.hpp"

#include <vector>
#include <cassert>
#include <set>

#include "graph.hpp"
#include "mcs.hpp"
#include "utils.hpp"

namespace triangulator {
namespace comb_enumerator {
namespace {

void dfs2(const Graph& graph, int x, std::vector<int>& u) {
  assert(u[x] == 0);
  u[x] = 1;
  for (int nx : graph.Neighbors(x)) {
    if (u[nx] == 0) dfs2(graph, nx, u);
    else if (u[nx] == 2) {
      u[nx] = 3;
    }
  }
}

bool extend2(const Graph& graph, const std::vector<int>& pmc, int x) {
  std::vector<int> u(graph.n());
  for (int v : pmc) u[v] = 2;
  dfs2(graph, x, u);
  for (int v : pmc) {
    if (u[v] != 3) return false;
  }
  return true;
}

int dfs1(const Graph& graph, int x, std::vector<int>& u, int it) {
  assert(u[x] == 0);
  u[x] = 1;
  int r = 0;
  for (int nx : graph.Neighbors(x)) {
    if (u[nx] == 0) r += dfs1(graph, nx, u, it);
    else if (u[nx] > 1 && u[nx] < it) {
      u[nx] = it;
      r++;
    }
  }
  return r;
}

bool HasFullComponent(const Graph& graph, const std::vector<int>& sep) {
  std::vector<int> u(graph.n());
  int it = 2;
  for (int x : sep) {
    u[x] = it;
  }
  for (int i = 0; i < graph.n(); i++) {
    if (u[i] == 0) {
      it++;
      int c = dfs1(graph, i, u, it);
      assert(c <= (int)sep.size());
      if (c == (int)sep.size()) return true;
    }
  }
  return false;
}

bool IsPmc(const Graph& graph, const std::vector<int>& pmc) {
  if (HasFullComponent(graph, pmc)) return false;
  std::vector<int> u(graph.n());
  for (int x : pmc) {
    std::fill(u.begin(), u.end(), 0);
    for (int y : pmc) {
      if (y != x) {
        u[y] = 2;
      }
    }
    dfs2(graph, x, u);
    for (int y : pmc) {
      if (y != x) {
        if (u[y] != 3) return false;
      }
    }
  }
  return true;
}

std::vector<std::vector<int>> OneMoreVertex(const Graph& new_graph, const std::vector<std::vector<int>>& pmcs, 
  const std::vector<std::vector<int>>& minseps, const std::vector<std::vector<int>>& new_minseps, int x) {
  std::vector<std::vector<int>> new_pmcs;
  for (const auto& pmc : pmcs) {
    if (!HasFullComponent(new_graph, pmc)) {
      new_pmcs.push_back(pmc);
    } else {
      if (extend2(new_graph, pmc, x)) {
        new_pmcs.push_back(pmc);
        new_pmcs.back().push_back(x);
      }
    }
  }
  int i2 = 0;
  std::set<std::vector<int>> tried;
  for (const auto& minsep : new_minseps) {
    if (std::find(minsep.begin(), minsep.end(), x) != minsep.end()) continue;
    if (extend2(new_graph, minsep, x)) {
      new_pmcs.push_back(minsep);
      new_pmcs.back().push_back(x);
      if (HasFullComponent(new_graph, new_pmcs.back())) {
        new_pmcs.pop_back();
      }
    }
    while (i2 < (int)minseps.size() && minseps[i2] < minsep) i2++;
    if (i2 < (int)minseps.size() && minseps[i2] == minsep) continue;
    std::vector<std::vector<int>> cs = new_graph.Components(minsep);
    assert(cs.size() >= 2);
    for (const auto& minsep2 : new_minseps) {
      for (auto& c : cs) {
        std::sort(c.begin(), c.end());
        std::vector<int> npmc = minsep;
        for (int y : minsep2) {
          if (binary_search(c.begin(), c.end(), y)) {
            if (!binary_search(minsep.begin(), minsep.end(), y)) {
              npmc.push_back(y);
            }
          }
        }
        if (npmc.size() > minsep.size()) {
          std::sort(npmc.begin(), npmc.end());
          if (tried.count(npmc)) continue;
          tried.insert(npmc);
          if (IsPmc(new_graph, npmc)) {
            new_pmcs.push_back(npmc);
          }
        }
      }
    }
  }
  return new_pmcs;
}
  
} // namespace

std::vector<std::vector<int>> FindMinSeps(const Graph& graph, int ub) {
  std::vector<std::vector<int>> minseps;
  std::set<std::vector<int>> ff;
  std::vector<int> u(graph.n());
  for (int i = 0; i < graph.n(); i++) {
    if (graph.Neighbors(i).empty()) continue;
    auto block = graph.Neighbors(i);
    block.push_back(i);
    auto cs = graph.Components(block);
    for (const auto& c : cs) {
      auto nbs = graph.Neighbors(c);
      std::sort(nbs.begin(), nbs.end());
      if (!ff.count(nbs)) {
        ff.insert(nbs);
        minseps.push_back(nbs);
      }
    }
  }
  for (int i = 0; i < (int)minseps.size(); i++) {
    if (ub != -1 && (int)minseps.size() >= ub) break;
    auto minsep = minseps[i];
    for (int x : minsep) {
      std::vector<int> block;
      block.reserve(minsep.size() + graph.Neighbors(x).size());
      for (int v : minsep) block.push_back(v);
      for (int v : graph.Neighbors(x)) block.push_back(v);
      auto cs = graph.Components(block);
      for (const auto& c : cs) {
        auto nbs = graph.Neighbors(c);
        std::sort(nbs.begin(), nbs.end());
        if (!ff.count(nbs)) {
          ff.insert(nbs);
          minseps.push_back(nbs);
        }
      }
    }
  }
  std::sort(minseps.begin(), minseps.end());
  return minseps;
}

std::vector<std::vector<int>> Pmcs(const Graph& graph) {
  if (graph.n() == 0) return {};
  assert(graph.IsConnectedOrIsolated());
  std::vector<int> order = mcs::Mcs(graph);
  std::reverse(order.begin(), order.end());
  std::vector<int> ord(graph.n());
  for (int i = 0; i < graph.n(); i++) {
    ord[order[i]] = i;
  }
  Graph new_graph(graph.n());
  std::vector<std::vector<int>> pmcs = {{order[0]}};
  std::vector<std::vector<int>> minseps = {};
  for (int i = 1; i < graph.n(); i++) {
    int x = order[i];
    for (int nx : graph.Neighbors(x)) {
      if (ord[nx] < ord[x]) {
        new_graph.AddEdge(nx, x);
      }
    }
    assert(new_graph.IsConnectedOrIsolated());
    auto new_minseps = FindMinSeps(new_graph, -1);
    pmcs = OneMoreVertex(new_graph, pmcs, minseps, new_minseps, x);
    minseps = new_minseps;
    for (auto& pmc : pmcs) {
      std::sort(pmc.begin(), pmc.end());
    }
    utils::SortAndDedup(pmcs);
  }
  return pmcs;
}
} // namespace comb_enumerator
} // namespace triangulator
