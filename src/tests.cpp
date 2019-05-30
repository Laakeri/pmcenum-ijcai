#include <fstream>
#include <vector>

#include "graph.hpp"
#include "io.hpp"
#include "hypergraph.hpp"
#include "solver.hpp"
#include "utils.hpp"

namespace triangulator {
namespace {
  std::vector<std::string> sat_solvers_ = {"minisat", "cryptominisat", "ipasir", "glucose", "mapleglucose", "minisatnopp", "glucosenopp"};
}

void testHyperGraph(std::string filename, int ghtw) {
  Io io;
  std::ifstream input("hyper_instances/"+filename);
  const HyperGraph hypergraph = io.ReadHyperGraph(input);
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 2; j++) {
      for (std::string solver : sat_solvers_) {
        int sat_ghtw = HypertreewidthSat(hypergraph, i, j, solver);
        if (sat_ghtw != ghtw) {
          utils::ErrorDie("Fail ", filename, " ", solver, ". Got ", sat_ghtw, " expected ", ghtw);
        }
      }
    }
  }
  int asp_ghtw = HypertreewidthAsp(hypergraph);
  if (asp_ghtw != ghtw) {
    utils::ErrorDie("Fail ", filename, " ASP. Got ", asp_ghtw, " expected ", ghtw);
  }
  int comb_ghtw = HypertreewidthComb(hypergraph);
  if (comb_ghtw != ghtw) {
    utils::ErrorDie("Fail ", filename, " comb. Got ", comb_ghtw, " expected ", ghtw);
  }
  Log::Write(2, "Success ", filename, " ", ghtw);
}

void testGraph(std::string filename, int tw) {
  Io io;
  std::ifstream input("instances/"+filename);
  const Graph graph = io.ReadGraph(input);
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 2; j++) {
      for (std::string solver : sat_solvers_) {
        int sat_tw = TreewidthSat(graph, i, j, solver, true);
        if (sat_tw != tw) {
          utils::ErrorDie("Fail ", filename, " ", solver, ". Got ", sat_tw, " expected ", tw);
        }
      }
    }
  }
  // int asp_tw = TreewidthAsp(graph, true);
  // if (asp_tw != tw) {
  //   utils::ErrorDie("Fail ", filename, " ASP. Got ", asp_tw, " expected ", tw);
  // }
  int comb_tw = TreewidthComb(graph, true);
  if (comb_tw != tw) {
    utils::ErrorDie("Fail ", filename, " comb. Got ", comb_tw, " expected ", tw);
  }
  Log::Write(2, "Success ", filename, " ", tw);
}

void runtestsFast() {
  // Runtime of should be <20s.
  Log::SetLogLevel(2);
  testGraph("weird1.graph", 0);
  testGraph("weird2.graph", 1);
  testGraph("weird3.graph", 0);
  testGraph("weird4.graph", 1);
  testGraph("weird5.graph", 2);
  testGraph("weird6.graph", 2);
  testGraph("weird7.graph", 1);
  testGraph("weird8.graph", 2);
  testGraph("anna-pp.graph", 12);
  testGraph("alarm.graph", 4);
  testGraph("celar09pp.graph", 7);
  testGraph("child.graph", 3);
  testGraph("grid4_4.graph", 4);
  testGraph("hepar2.graph", 6);
  testGraph("huck.graph", 10);
  testGraph("mildew.graph", 4);
  testGraph("myciel2.graph", 2);
  testGraph("myciel3.graph", 5);
  testGraph("oesoca.graph", 3);
  testGraph("oesoca+-pp.graph", 11);
  testGraph("oesoca42.graph", 3);
  testGraph("pace16tw_BlanusaSecondSnarkGraph.graph", 4);
  testGraph("pace16tw_ChvatalGraph.graph", 6);

  testHyperGraph("s27.graph", 2);
}

void runtestsSlow() {
  // Runtime should be <20min.
  Log::SetLogLevel(2);
  testGraph("anna.graph", 12);
  testGraph("barley.graph", 7);
  testGraph("david-pp.graph", 13);
  testGraph("hailfinder.graph", 4);
  testGraph("jean.graph", 9);
  testGraph("mainuk.graph", 7);
  testGraph("pace16tw_AhrensSzekeresGeneralizedQuadrangleGraph_3.graph", 17);
  testGraph("pace16tw_BrinkmannGraph.graph", 8);
  
  testHyperGraph("grid5.graph", 3);
}

} // namespace triangulator
