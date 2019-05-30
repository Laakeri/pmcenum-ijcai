#include "graph.hpp"

#include <vector>
#include <algorithm>
#include <set>
#include <cassert>
#include <ostream>

#include "utils.hpp"
#include "matrix.hpp"

namespace triangulator {

Graph::Graph(int n)
  : n_(n), m_(0), adj_list_(n), adj_mat_(n, n) {
  std::vector<int> identity(n);
  for (int i = 0; i < n; i++) identity[i] = i;
  vertex_map_.Init(identity);
}

Graph::Graph(std::vector<Edge> edges) : vertex_map_(edges) {
  n_ = vertex_map_.Size();
  m_ = 0;
  adj_list_.resize(n_);
  adj_mat_.Resize(n_, n_);
  for (auto edge : edges) {
    AddEdge(vertex_map_.Rank(edge.first), vertex_map_.Rank(edge.second));
  }
}

int Graph::n() const {
  return n_;
}

int Graph::m() const {
  return m_;
}

bool Graph::HasEdge(int v, int u) const {
  return adj_mat_[v][u];
}

bool Graph::HasEdge(Edge e) const {
  return HasEdge(e.first, e.second);
}

std::vector<Edge> Graph::Edges() const {
  std::vector<Edge> ret;
  for (int i = 0; i < n_; i++) {
    for (int a : adj_list_[i]) {
      if (a > i) ret.push_back({i, a});
    }
  }
  return ret;
}

const std::vector<int>& Graph::Neighbors(int v) const {
  return adj_list_[v];
}

bool Graph::IsConnectedOrIsolated() const {
  auto cs = Components({});
  int f = 0;
  for (const auto& c : cs) {
    if ((int)c.size() > 1) f++;
  }
  return f <= 1;
}

void Graph::AddEdge(int v, int u) {
  if (adj_mat_[v][u]) return;
  assert(v != u);
  m_++;
  adj_mat_[v][u] = true;
  adj_mat_[u][v] = true;
  adj_list_[v].push_back(u);
  adj_list_[u].push_back(v);
}

void Graph::AddEdge(Edge e) {
  AddEdge(e.first, e.second);
}

void Graph::AddEdges(const std::vector<Edge>& edges) {
  for (auto& edge : edges) AddEdge(edge);
}

void Graph::RemoveEdge(int v, int u) {
  assert(adj_mat_[v][u] && adj_mat_[u][v]);
  m_--;
  adj_mat_[v][u] = false;
  adj_mat_[u][v] = false;
  int fo = 0;
  for (int i = 0; i < adj_list_[v].size(); i++) {
    if (adj_list_[v][i] == u) {
      std::swap(adj_list_[v][i], adj_list_[v].back());
      adj_list_[v].pop_back();
      fo++;
      break;
    }
  }
  for (int i = 0; i < adj_list_[u].size(); i++) {
    if (adj_list_[u][i] == v) {
      std::swap(adj_list_[u][i], adj_list_[u].back());
      adj_list_[u].pop_back();
      fo++;
      break;
    }
  }
  assert(fo == 2);
}

int Graph::MinDegree() const {
  int mdg = n_ - 1;
  for (int i = 0; i < n_; i++) {
    mdg = std::min(mdg, (int)adj_list_[i].size());
  }
  return mdg;
}

void Graph::Dfs(int v, std::vector<char>& block, std::vector<int>& component) const {
  block[v] = true;
  component.push_back(v);
  for (int nv : adj_list_[v]) {
    if (!block[nv]) {
      Dfs(nv, block, component);
    }
  }
}

std::vector<int> Graph::FindComponentAndMark(int v, std::vector<char>& block) const {
  std::vector<int> component;
  Dfs(v, block, component);
  return component;
}

std::vector<std::vector<int> > Graph::Components(const std::vector<int>& separator) const {
  std::vector<char> blocked(n_);
  for (int v : separator) {
    blocked[v] = true;
  }
  std::vector<std::vector<int> > components;
  for (int i = 0; i < n_; i++) {
    if (!blocked[i]) {
      components.push_back(FindComponentAndMark(i, blocked));
    }
  }
  return components;
}

// Returns the vector in sorted order
std::vector<int> Graph::Neighbors(const std::vector<int>& vs) const {
  std::vector<int> neighbors;
  int sum = 0;
  for (int v : vs) {
    sum += adj_list_[v].size();
  }
  if (sum >= n_/4) { // Two cases for optimization
    std::vector<char> nbs(n_);
    for (int v : vs) {
      for (int nv : adj_list_[v]) {
        nbs[nv] = true;
      }
    }
    for (int v : vs) {
      nbs[v] = false;
    }
    for (int i = 0; i < n_; i++) {
      if (nbs[i]) neighbors.push_back(i);
    }
  } else {
    std::set<int> nbs;
    for (int v : vs) {
      for (int nv : adj_list_[v]) {
        nbs.insert(nv);
      }
    }
    for (int v : vs) {
      nbs.erase(v);
    }
    for (int v : nbs) {
      neighbors.push_back(v);
    }
  }
  return neighbors;
}

std::vector<Edge> Graph::EdgesIn(const std::vector<int>& vs) const {
  static std::vector<char> is;
  utils::InitZero(is, n_);
  for (int v : vs) {
    is[v] = true;
  }
  std::vector<Edge> edges;
  for (int v : vs) {
    if (adj_list_[v].size() <= vs.size()) { // Two cases for optimization
      for (int nv : adj_list_[v]) {
        if (is[nv] && nv > v) edges.push_back({v, nv});
      }
    }
    else {
      for (int nv : vs) {
        if (adj_mat_[v][nv] && nv > v) edges.push_back({v, nv});
      }
    }
  }
  return edges;
}

Matrix<char> Graph::ConnectedMatrix(const std::vector<int>& separator) const {
  Matrix<char> connected = adj_mat_;
  std::vector<std::vector<int> > components = Components(separator);
  for (auto component : components) {
    std::vector<int> neighbors = Neighbors(component);
    component.insert(component.end(), neighbors.begin(), neighbors.end());
    for (int v : component) {
      for (int u : component) {
        connected[v][u] = true;
      }
    }
  }
  return connected;
}

bool Graph::IsPmc(const std::vector<int>& pmc) const {
  Matrix<char> connected = ConnectedMatrix(pmc);
  std::vector<char> in_pmc(n_);
  for (int v : pmc) {
    in_pmc[v] = true;
  }
  for (int i = 0; i < n_; i++) {
    if (in_pmc[i]) {
      for (int v : pmc) {
        if (i != v && !connected[i][v] && !adj_mat_[i][v]) return false;
      }
    } else {
      int connected_to = 0;
      for (int j = 0; j < n_; j++) {
        if (connected[i][j] && in_pmc[j]) connected_to++;
      }
      assert(connected_to <= pmc.size());
      if (connected_to == pmc.size()) return false;
    }
  }
  return true;
}

bool Graph::IsClique(const std::vector<int>& clique) const {
  for (int i = 0; i < clique.size(); i++) {
    for (int ii = i + 1; ii < clique.size(); ii++) {
      if (!adj_mat_[clique[i]][clique[ii]]) return false;
    }
  }
  return true;
}

int Graph::MapBack(int v) const {
  return vertex_map_.Kth(v);
}
std::vector<int> Graph::MapBack(std::vector<int> vs) const {
  for (int& v : vs) {
    v = MapBack(v);
  }
  return vs;
}
int Graph::MapInto(int v) const {
  return vertex_map_.Rank(v);
}
std::vector<int> Graph::MapInto(std::vector<int> vs) const {
  for (int& v : vs) {
    v = MapInto(v);
  }
  return vs;
}
Edge Graph::MapBack(Edge e) const {
  return {MapBack(e.first), MapBack(e.second)};
}
std::vector<Edge> Graph::MapBack(std::vector<Edge> es) const {
  for (Edge& e : es) {
    e = MapBack(e);
  }
  return es;
}
void Graph::InheritMap(const Graph& parent) {
  vertex_map_ = StaticSet<int>(parent.MapBack(vertex_map_.Values()));
}
void Graph::Print(std::ostream& out) const {
  out<<"v e: "<<n_<<" "<<m_<<std::endl;
  for (int i = 0; i < n_; i++) {
    for (int a : adj_list_[i]) {
      out<<i<<" "<<a<<std::endl;
    }
  }
}
} // namespace triangulator                                           
