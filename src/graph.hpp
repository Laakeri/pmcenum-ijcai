#pragma once

#include <vector>
#include <ostream>

#include "utils.hpp"
#include "staticset.hpp"
#include "matrix.hpp"

namespace triangulator {

typedef std::pair<int, int> Edge;

class Graph {
public:
  explicit Graph(int n);
  explicit Graph(std::vector<Edge> edges);
  void AddEdge(int v, int u);
  void AddEdge(Edge e);
  void AddEdges(const std::vector<Edge>& edges);

  void RemoveEdge(int v, int u);
  
  int n() const;
  int m() const;
  bool HasEdge(int v, int u) const;
  bool HasEdge(Edge e) const;
  std::vector<Edge> Edges() const;

  int MinDegree() const;
  
  bool IsConnectedOrIsolated() const;
  
  const std::vector<int>& Neighbors(int v) const;
  std::vector<std::vector<int> > Components(const std::vector<int>& separator) const;
  std::vector<int> Neighbors(const std::vector<int>& vs) const;
  std::vector<int> FindComponentAndMark(int v, std::vector<char>& block) const;
  std::vector<Edge> EdgesIn(const std::vector<int>& vs) const;
  // Two vertices are connected wrt. separator if there exists a path between them where all intermediate vertices are outside of the separator
  Matrix<char> ConnectedMatrix(const std::vector<int>& separator) const;
  
  bool IsPmc(const std::vector<int>& pmc) const;
  bool IsClique(const std::vector<int>& clique) const;
  
  int MapBack(int v) const;
  std::vector<int> MapBack(std::vector<int> vs) const;
  Edge MapBack(Edge e) const;
  std::vector<Edge> MapBack(std::vector<Edge> es) const;
  int MapInto(int v) const;
  std::vector<int> MapInto(std::vector<int> vs) const;
  
  void InheritMap(const Graph& parent);

  void Print(std::ostream& out) const;
  
  Graph(const Graph& rhs) = default;
  Graph& operator=(const Graph& rhs) = default;
  
private:
  int n_, m_;
  StaticSet<int> vertex_map_;
  std::vector<std::vector<int> > adj_list_;
  Matrix<char> adj_mat_;
  void Dfs(int v, std::vector<char>& blocked, std::vector<int>& component) const;
};
} // namespace triangulator
