#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <iomanip>
#include <set>
#include <cassert>

#include "graph.hpp"
#include "utils.hpp"
#include "io.hpp"
#include "tests.hpp"
#include "solver.hpp"

int main(int argc, char** argv) {
  if (argc < 2) triangulator::utils::ErrorDie("Give problem as the first argument.");
  std::string farg(argv[1]);
  std::set<std::string> solvers = {"minisat", "cryptominisat", "ipasir", "asp", "comb", "glucose", "mapleglucose", "glucosenopp", "minisatnopp"};
  triangulator::Log::SetLogLevel(3);
  std::cerr<<std::setprecision(3)<<std::fixed;
  if (farg == "runtests") {
    triangulator::runtestsFast();
  } else if (farg == "runtestsall") {
    triangulator::runtestsFast();
    triangulator::runtestsSlow();
  } else if (farg == "tw" || farg == "twnopp") {
    bool pp = true;
    if (farg == "twnopp") pp = false;
    assert(argc >= 4);
    std::string solver(argv[2]);
    std::string file(argv[3]);
    assert(solvers.find(solver) != solvers.end());
    triangulator::Io io;
    std::ifstream input(file);
    triangulator::Graph graph = io.ReadGraph(input);
    int sol;
    if (solver == "asp"){
      assert(argc == 4);
      sol = triangulator::TreewidthAsp(graph, pp);
    } else if (solver == "comb") {
      assert(argc == 4);
      sol = triangulator::TreewidthComb(graph, pp);
    } else {
      assert(argc == 6);
      int ms_enc = std::stoi(argv[4]);
      int card_enc = std::stoi(argv[5]);
      assert(ms_enc >= 0 && ms_enc <= 5);
      assert(card_enc >= 0 && card_enc <= 1);
      sol = triangulator::TreewidthSat(graph, ms_enc, card_enc, solver, pp);
    }
    std::cout << sol << std::endl;
  } else if (farg == "ghtw") {
    assert(argc >= 4);
    std::string solver(argv[2]);
    std::string file(argv[3]);
    assert(solvers.find(solver) != solvers.end());
    triangulator::Io io;
    std::ifstream input(file);
    triangulator::HyperGraph hypergraph = io.ReadHyperGraph(input);
    int sol;
    if (solver == "asp") {
      assert(argc == 4);
      sol = triangulator::HypertreewidthAsp(hypergraph);
    } else if (solver == "comb") {
      assert(argc == 4);
      sol = triangulator::HypertreewidthComb(hypergraph);
    } else {
      assert(argc == 6);
      int ms_enc = std::stoi(argv[4]);
      int card_enc = std::stoi(argv[5]);
      assert(ms_enc >= 0 && ms_enc <= 5);
      assert(card_enc >= 0 && card_enc <= 1);
      sol = triangulator::HypertreewidthSat(hypergraph, ms_enc, card_enc, solver);
    }
    std::cout << sol << std::endl;
  } else if (farg == "countminseps") {
    assert(argc == 4);
    int ub = std::stoi(argv[2]);
    assert(ub>1);
    std::string file(argv[3]);
    triangulator::Io io;
    std::ifstream input(file);
    triangulator::Graph graph = io.ReadGraph(input);
    int sol = triangulator::CountMinseps(graph, ub);
    std::cout << sol << std::endl;
  } else {
    abort();
  }
}
