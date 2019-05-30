#include "io.hpp"

#include <vector>
#include <string>
#include <istream>
#include <sstream>
#include <algorithm>
#include <cassert>

#include "graph.hpp"
#include "utils.hpp"

namespace triangulator {

int NumTokens(std::string s) {
  std::stringstream ss;
  ss<<s;
  int num = 0;
  while (ss>>s) {
    num++;
  }
  return num;
}

std::vector<std::string> GetTokens(std::string s) {
  std::stringstream ss;
  ss<<s;
  std::vector<std::string> tokens;
  while (ss>>s) {
    tokens.push_back(s);
  }
  return tokens;
}

Graph Io::ReadGraph(std::istream& in) {
  std::vector<std::pair<std::string, std::string> > edges;
  std::string input_line;
  bool format_detected = false;
  dimacs_ = false;
  int line_num = 0;
  while (std::getline(in, input_line)) {
    line_num++;
    assert(input_line.size() > 0);
    if (input_line.size() >= 2 && input_line.substr(0, 2) == "p " && NumTokens(input_line) >= 3) {
      if (format_detected) {
        utils::Warning("Detected another begin of a file format in line ", line_num, " lines before that are ignored");
      }
      Log::Write(10, "Dimacs graph format detected");
      edges.clear();
      dimacs_ = true;
      format_detected = true;
    } else if (dimacs_ && input_line.size() >= 2 && input_line.substr(0, 2) == "e " && NumTokens(input_line) == 3) {
      auto tokens = GetTokens(input_line);
      assert(tokens[0] == "e");
      edges.push_back({tokens[1], tokens[2]});
    }
  }
  vertex_map_.Init(edges);
  Graph graph(vertex_map_.Size());
  for (auto edge : edges) {
    graph.AddEdge(vertex_map_.Rank(edge.first), vertex_map_.Rank(edge.second));
  }
  return graph;
}
HyperGraph Io::ReadHyperGraph(std::istream& in) {
  std::vector<std::string> vertices;
  std::vector<std::vector<std::string> > edges;
  std::string in_e;
  while (getline(in, in_e)) {
    for (char& c : in_e) {
      if (c == ',') c = ' ';
    }
    std::stringstream ss;
    ss<<in_e;
    std::string ve;
    std::vector<std::string> te;
    while (ss>>ve) {
      te.push_back(ve);
      vertices.push_back(ve);
    }
    utils::SortAndDedup(te);
    edges.push_back(te);
  }
  vertex_map_.Init(vertices);
  HyperGraph graph(vertex_map_.Size());
  for (auto& edge : edges) {
    std::vector<int> e;
    for (auto& v : edge) {
      e.push_back(vertex_map_.Rank(v));
    }
    graph.AddEdge(e);
  }
  return graph;
}

} // namespace triangulator
