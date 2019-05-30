#include "asp_enumerator.hpp"

#include <vector>
#include <fstream>
#include <cassert>
#include <chrono>
#include <cstdlib>

#include "graph.hpp"
#include "hypergraph.hpp"
#include "utils.hpp"

namespace triangulator {
namespace {
std::string TmpInstance(int a, int b, int c) {
  std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  uint64_t micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
  return "tmp/instance"+std::to_string(micros)+"_"+std::to_string(a)+"_"+std::to_string(b)+"_"+std::to_string(c)+".lp";
}
} // namespace

namespace asp_enumerator {
std::vector<std::vector<int>> TreewidthPmcs(const Graph& graph, int size) {
  std::string tmp_file_in = TmpInstance(graph.n(), graph.m(), size);
  std::string tmp_file_out = TmpInstance(graph.n(), graph.m(), size+1000000);
  std::ofstream out(tmp_file_in);
  out<<"vertices("<<graph.n()<<")."<<std::endl;
  out<<"size("<<size<<")."<<std::endl;
  for (const Edge& e : graph.Edges()) {
  	out<<"edge("<<e.first+1<<","<<e.second+1<<")."<<std::endl;
  }
  out.close();
  system(("solvers/clingo/clingo "+tmp_file_in+" solvers/clingo/treewidth_encoding.lp 0 > "+tmp_file_out).c_str());
  std::ifstream in(tmp_file_out);
  std::vector<std::vector<int>> ret;
  std::string tmp;
  while (std::getline(in, tmp)) {
    if ((int)tmp.size() > 7 && tmp.substr(0, 7) == "Answer:") {
      std::getline(in, tmp);
      assert((int)tmp.size() >= 6);
      bool paren = false;
      std::string num;
      std::vector<int> pmc;
      for (char c : tmp) {
        if (c == '(') {
          num = "";
          paren = true;
        } else if (c == ')') {
          assert(paren);
          int tv = stoi(num) - 1;
          assert(tv >= 0 && tv < graph.n());
          pmc.push_back(tv);
          paren = false;
        } else if (paren) {
          num += c;
        }
      }
      assert(!paren);
      assert((int)pmc.size() > 0);
      ret.push_back(pmc);
    }
  }
  in.close();
  system(("rm "+tmp_file_in).c_str());
  system(("rm "+tmp_file_out).c_str());
  return ret;
}
std::vector<std::vector<int>> HypertreewidthPmcs(const HyperGraph& hypergraph, int size) {
  std::string tmp_file_in = TmpInstance(hypergraph.n(), hypergraph.m(), size+100);
  std::string tmp_file_out = TmpInstance(hypergraph.n(), hypergraph.m(), size+1000);
  std::ofstream out(tmp_file_in);
  out<<"vertices("<<hypergraph.n()<<")."<<std::endl;
  out<<"hyperedges("<<hypergraph.m()<<")."<<std::endl;
  out<<"size("<<size<<")."<<std::endl;
  for (int i = 0; i < hypergraph.m(); i++) {
    for (int v : hypergraph.Edges()[i]) {
      out<<"inedge("<<v+1<<", "<<i+1<<")."<<std::endl;
    }
  }
  for (const Edge& e : hypergraph.PrimalGraph().Edges()) {
    out<<"edge("<<e.first+1<<","<<e.second+1<<")."<<std::endl;
  }
  out.close();
  system(("solvers/clingo/clingo "+tmp_file_in+" solvers/clingo/hypertreewidth_encoding.lp 0 > "+tmp_file_out).c_str());
  std::ifstream in(tmp_file_out);
  std::vector<std::vector<int>> ret;
  std::string tmp;
  while (std::getline(in, tmp)) {
    if ((int)tmp.size() > 7 && tmp.substr(0, 7) == "Answer:") {
      std::getline(in, tmp);
      assert((int)tmp.size() >= 6);
      bool paren = false;
      std::string num;
      std::vector<int> pmc;
      for (char c : tmp) {
        if (c == '(') {
          num = "";
          paren = true;
        } else if (c == ')') {
          assert(paren);
          int tv = stoi(num) - 1;
          assert(tv >= 0 && tv < hypergraph.n());
          pmc.push_back(tv);
          paren = false;
        } else if (paren) {
          num += c;
        }
      }
      assert(!paren);
      assert((int)pmc.size() > 0);
      ret.push_back(pmc);
    }
  }
  in.close();
  for (auto& pmc : ret) {
    sort(pmc.begin(), pmc.end());
  }
  utils::SortAndDedup(ret);
  system(("rm "+tmp_file_in).c_str());
  system(("rm "+tmp_file_out).c_str());
  return ret;
}
} // namespace asp_enumerator
} // namespace triangulator
