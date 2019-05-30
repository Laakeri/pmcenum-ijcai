#include "solver.hpp"

#include <vector>
#include <memory>

#include "minisat_interface.hpp"
#include "cryptominisat_interface.hpp"
#include "ipasir_interface.hpp"
#include "enumerator.hpp"
#include "graph.hpp"
#include "utils.hpp"
#include "io.hpp"
#include "treewidthpreprocessor.hpp"
#include "bt_algorithm.hpp"
#include "fixed_size_enumerator.hpp"
#include "fixed_size_hyper_enumerator.hpp"
#include "cardinality_constraint_builder.hpp"
#include "asp_enumerator.hpp"
#include "comb_enum.hpp"
#include "hypergraph.hpp"
#include "hypertreewidthpreprocessor.hpp"
#include "setcover.hpp"
#include "glucose_interface.hpp"
#include "mapleglucose_interface.hpp"

namespace triangulator {

namespace {
std::shared_ptr<SatInterface> SatSolver(const std::string& name) {
  if (name == "minisat") {
    return std::make_shared<MinisatInterface>(true);
  } else if (name == "cryptominisat") {
    return std::make_shared<CryptominisatInterface>();
  } else if (name == "ipasir") {
    return std::make_shared<IpasirInterface>();
  } else if (name == "glucose") {
    return std::make_shared<GlucoseInterface>(true);
  } else if (name == "mapleglucose") {
    return std::make_shared<MapleGlucoseInterface>(true);
  } else if (name == "glucosenopp") {
    return std::make_shared<GlucoseInterface>(false);
  } else if (name == "minisatnopp") {
    return std::make_shared<MinisatInterface>(false);
  } else {
    assert(false);
  }
}
} // namespace

int TreewidthSat(const Graph& graph, int minsep_enconding, int card_encoding, const std::string& solver, bool pp) {
  Log::Write(3, "i solver_param tw sat ", minsep_enconding, " ", solver);
  Log::Write(3, "i graph_size ", graph.n(), " ", graph.m());
  Timer pp_timer;
  pp_timer.start();
  TreewidthPreprocessor twpp(graph);
  std::vector<TreewidthInstance> instances = twpp.Preprocess(pp);
  pp_timer.stop();
  Log::Write(3, "i pp_time ", pp_timer.getTime().count());
  std::vector<TreewidthSolution> solutions;
  Timer sat_timer;
  Timer bt_timer;
  for (const auto& instance : instances) {
    Log::Write(3, "i pp_size ", instance.graph.n(), " ", instance.graph.m());
    std::vector<std::vector<int> > apmcs;
    sat_timer.start();
    FixedSizeEnumerator e(instance.graph, SatSolver(solver), minsep_enconding, card_encoding);
    sat_timer.stop();
    for (int k = 1; k <= instance.graph.n(); k++) {
      sat_timer.start();
      auto pmcs = e.AllPmcs(k);
      sat_timer.stop();
      Log::Write(3, "i pmcs ", k, " ", pmcs.size());
      Log::Write(3, "i minseps ", k, " ", e.Stats().MinsepsFound());
      Log::Write(3, "i satcalls ", k, " ", e.Stats().SatCalls());
      for (const auto& pmc : pmcs) {
        assert(pmc.size() == k);
        apmcs.push_back(pmc);
      }
      bt_timer.start();
      BtAlgorithm bt;
      auto sol_b = bt.Solve(instance.graph, apmcs);
      bt_timer.stop();
      auto solution = TreewidthSolution{sol_b.second, sol_b.first};
      if (solution.treewidth >= 0) {
        assert(solution.treewidth == k-1);
        solutions.push_back(solution);
        break;
      }
    }
  }
  Log::Write(3, "i sat_time ", sat_timer.getTime().count());
  Log::Write(3, "i bt_time ", bt_timer.getTime().count());
  auto solution = twpp.MapBack(solutions);
  return solution.treewidth;
}

int TreewidthAsp(const Graph& graph, bool pp) {
  Log::Write(3, "i solver_param tw asp ");
  Log::Write(3, "i graph_size ", graph.n(), " ", graph.m());
  Timer pp_timer;
  pp_timer.start();
  TreewidthPreprocessor twpp(graph);
  std::vector<TreewidthInstance> instances = twpp.Preprocess(pp);
  pp_timer.stop();
  Log::Write(3, "i pp_time ", pp_timer.getTime().count());
  std::vector<TreewidthSolution> solutions;
  Timer asp_timer;
  Timer bt_timer;
  for (const auto& instance : instances) {
    Log::Write(3, "i pp_size ", instance.graph.n(), " ", instance.graph.m());
    std::vector<std::vector<int>> apmcs;
    for (int k = 1; k <= instance.graph.n(); k++) {
      asp_timer.start();
      auto pmcs = asp_enumerator::TreewidthPmcs(instance.graph, k);
      asp_timer.stop();
      Log::Write(3, "i pmcs ", k, " ", pmcs.size());
      for (const auto& pmc : pmcs) {
        assert(pmc.size() == k);
        apmcs.push_back(pmc);
      }
      bt_timer.start();
      BtAlgorithm bt;
      auto sol_b = bt.Solve(instance.graph, apmcs);
      bt_timer.stop();
      auto solution = TreewidthSolution{sol_b.second, sol_b.first};
      if (solution.treewidth >= 0) {
        assert(solution.treewidth == k-1);
        solutions.push_back(solution);
        break;
      }
    }
  }
  Log::Write(3, "i asp_time ", asp_timer.getTime().count());
  Log::Write(3, "i bt_time ", bt_timer.getTime().count());
  auto solution = twpp.MapBack(solutions);
  return solution.treewidth;
}

int TreewidthComb(const Graph& graph, bool pp) {
  Log::Write(3, "i solver_param tw comb");
  Log::Write(3, "i graph_size ", graph.n(), " ", graph.m());
  Timer pp_timer;
  pp_timer.start();
  TreewidthPreprocessor twpp(graph);
  std::vector<TreewidthInstance> instances = twpp.Preprocess(pp);
  pp_timer.stop();
  Log::Write(3, "i pp_time ", pp_timer.getTime().count());
  std::vector<TreewidthSolution> solutions;
  Timer enum_timer;
  Timer bt_timer;
  for (const auto& instance : instances) {
    Log::Write(3, "i pp_size ", instance.graph.n(), " ", instance.graph.m());
    enum_timer.start();
    auto pmcs = comb_enumerator::Pmcs(instance.graph);
    enum_timer.stop();
    Log::Write(3, "i pmcs ", pmcs.size());
    bt_timer.start();
    BtAlgorithm bt;
    auto sol_b = bt.Solve(instance.graph, pmcs);
    bt_timer.stop();
    auto solution = TreewidthSolution{sol_b.second, sol_b.first};
    solutions.push_back(solution);
  }
  Log::Write(3, "i enum_time ", enum_timer.getTime().count());
  Log::Write(3, "i bt_time ", bt_timer.getTime().count());
  auto solution = twpp.MapBack(solutions);
  return solution.treewidth;
}

int HypertreewidthSat(const HyperGraph& hypergraph, int minsep_enconding, int card_encoding, const std::string& solver) {
  Log::Write(3, "i solver_param ghtw sat ", minsep_enconding, " ", solver);
  Log::Write(3, "i graph_size ", hypergraph.n(), " ", hypergraph.m(), " ", hypergraph.PrimalGraph().m());
  Timer pp_timer;
  pp_timer.start();
  HypertreewidthPreprocessor htwpp;
  std::vector<HyperGraph> instances = htwpp.Preprocess(hypergraph);
  pp_timer.stop();
  Log::Write(3, "i pp_time ", pp_timer.getTime().count());
  int solution = 1;
  Timer sat_timer;
  Timer bt_timer;
  for (const auto& instance : instances) {
    Log::Write(3, "i pp_size ", instance.n(), " ", instance.m(), " ", instance.PrimalGraph().m());
    std::vector<std::vector<int>> apmcs;
    sat_timer.start();
    FixedSizeHyperEnumerator e(instance, SatSolver(solver), minsep_enconding, card_encoding);
    sat_timer.stop();
    for (int k = 1; k <= instance.n(); k++) {
      sat_timer.start();
      auto pmcs = e.AllPmcs(k);
      sat_timer.stop();
      Log::Write(3, "i pmcs ", k, " ", pmcs.size());
      Log::Write(3, "i minseps ", k, " ", e.Stats().MinsepsFound());
      Log::Write(3, "i satcalls ", k, " ", e.Stats().SatCalls());
      for (const auto& pmc : pmcs) {
        apmcs.push_back(pmc);
      }
      bt_timer.start();
      BtAlgorithm bt;
      auto sol_b = bt.Solve(instance.PrimalGraph(), apmcs);
      bt_timer.stop();
      if (sol_b.first >= 0) {
        solution = std::max(solution, k);
        break;
      }
    }
  }
  Log::Write(3, "i sat_time ", sat_timer.getTime().count());
  Log::Write(3, "i bt_time ", bt_timer.getTime().count());
  return solution;
}

int HypertreewidthAsp(const HyperGraph& hypergraph) {
  Log::Write(3, "i solver_param ghtw asp ");
  Log::Write(3, "i graph_size ", hypergraph.n(), " ", hypergraph.m(), " ", hypergraph.PrimalGraph().m());
  Timer pp_timer;
  pp_timer.start();
  HypertreewidthPreprocessor htwpp;
  std::vector<HyperGraph> instances = htwpp.Preprocess(hypergraph);
  pp_timer.stop();
  Log::Write(3, "i pp_time ", pp_timer.getTime().count());
  int solution = 1;
  Timer asp_timer;
  Timer bt_timer;
  for (const auto& instance : instances) {
    Log::Write(3, "i pp_size ", instance.n(), " ", instance.m(), " ", instance.PrimalGraph().m());
    for (int k = 1; k <= instance.n(); k++) {
      asp_timer.start();
      auto pmcs = asp_enumerator::HypertreewidthPmcs(instance, k);
      asp_timer.stop();
      Log::Write(3, "i pmcs ", k, " ", pmcs.size());
      bt_timer.start();
      BtAlgorithm bt;
      auto sol_b = bt.Solve(instance.PrimalGraph(), pmcs);
      bt_timer.stop();
      if (sol_b.first >= 0) {
        solution = std::max(solution, k);
        break;
      }
    }
  }
  Log::Write(3, "i asp_time ", asp_timer.getTime().count());
  Log::Write(3, "i bt_time ", bt_timer.getTime().count());
  return solution;
}

int HypertreewidthComb(const HyperGraph& hypergraph) {
  Log::Write(3, "i solver_param ghtw comb");
  Log::Write(3, "i graph_size ", hypergraph.n(), " ", hypergraph.m(), " ", hypergraph.PrimalGraph().m());
  Timer pp_timer;
  pp_timer.start();
  HypertreewidthPreprocessor htwpp;
  std::vector<HyperGraph> instances = htwpp.Preprocess(hypergraph);
  pp_timer.stop();
  Log::Write(3, "i pp_time ", pp_timer.getTime().count());
  int solution = 1;
  Timer enum_timer;
  Timer bt_timer;
  Timer sc_timer;
  for (const auto& instance : instances) {
    Log::Write(3, "i pp_size ", instance.n(), " ", instance.m(), " ", instance.PrimalGraph().m());
    enum_timer.start();
    auto pmcs = comb_enumerator::Pmcs(instance.PrimalGraph());
    enum_timer.stop();
    Log::Write(3, "i pmcs ", pmcs.size());
    sc_timer.start();
    Setcover sc(instance.Edges());
    sc_timer.stop();
    std::vector<char> can(pmcs.size());
    for (int k = 1; k <= instance.n(); k++) {
      std::vector<std::vector<int>> tpmcs;
      for (int i = 0; i < (int)pmcs.size(); i++) {
        if (can[i]) {
          tpmcs.push_back(pmcs[i]);
        } else {
          sc_timer.start();
          auto sol = sc.Solve(pmcs[i], k, k);
          sc_timer.stop();
          if (sol[0] != -1) {
            assert((int)sol.size() == k);
            tpmcs.push_back(pmcs[i]);
            can[i] = true;
          }
        }
      }
      bt_timer.start();
      BtAlgorithm bt;
      auto sol_b = bt.Solve(instance.PrimalGraph(), tpmcs);
      bt_timer.stop();
      if (sol_b.first >= 0) {
        solution = std::max(solution, k);
        break;
      }
    }
  }
  Log::Write(3, "i enum_time ", enum_timer.getTime().count());
  Log::Write(3, "i bt_time ", bt_timer.getTime().count());
  Log::Write(3, "i sc_time ", sc_timer.getTime().count());
  return solution;
}

int CountMinseps(const Graph& graph, int ub) {
  Log::Write(3, "i solver_param cms");
  Log::Write(3, "i graph_size ", graph.n(), " ", graph.m());
  Timer pp_timer;
  pp_timer.start();
  TreewidthPreprocessor twpp(graph);
  std::vector<TreewidthInstance> instances = twpp.Preprocess(true);
  pp_timer.stop();
  Log::Write(3, "i pp_time ", pp_timer.getTime().count());
  int ans = 0;
  for (const auto& instance : instances) {
    ans += (int)comb_enumerator::FindMinSeps(instance.graph, ub).size();
    if (ans > ub) break;
  }
  return std::min(ans, ub);
}
} // namespace triangulator