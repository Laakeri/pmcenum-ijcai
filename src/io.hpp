#pragma once

#include <vector>
#include <string>
#include <istream>
#include <map>

#include "graph.hpp"
#include "hypergraph.hpp"
#include "utils.hpp"

namespace triangulator {

// TODO

class Io {
public:
  Graph ReadGraph(std::istream& in);
  HyperGraph ReadHyperGraph(std::istream& in);
private:
  bool dimacs_;
  StaticSet<std::string> vertex_map_;
};
} // namespace triangulator
