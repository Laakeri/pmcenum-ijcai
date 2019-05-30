#pragma once

// Base class for enumerating PMCs with SAT.

#include <vector>
#include <map>
#include <memory>
#include <random>

#include "sat_interface.hpp"
#include "graph.hpp"

namespace triangulator {

class EnumeratorStats {
public:
  EnumeratorStats();
  void PmcFound();
  void MinsepFound();
  void SatCalled();
  int PmcsFound() const;
  int MinsepsFound() const;
  int SatCalls() const;
  void Print(int lvl) const;
private:
  int pmcs_found_, minseps_found_, sat_calls_;
};

class Enumerator {
public:
  Enumerator(const Graph& graph, std::shared_ptr<SatInterface> solver, int minsep_encoding);

  EnumeratorStats Stats() const;
  void PrintStats(int lvl) const;

  Enumerator(const Enumerator&) = delete;
  Enumerator& operator=(const Enumerator&) = delete;
protected:
  const std::vector<Lit>& XVars() const;
  std::vector<int> GetPmc(std::vector<Lit> assumptions, bool first_call);
private:
  std::vector<Lit> x_var_;
  Matrix<Lit> c_var_;
  std::shared_ptr<SatInterface> solver_;
  std::map<std::vector<int>, Lit> minsep_vars_;
  const Graph graph_;
  const int minsep_encoding_;
  EnumeratorStats enumerator_stats_;
  std::mt19937 random_gen_;

  void BuildBasicEncoding();
  void BuildPathLengthEncoding();
  void BlockBadSolution(std::vector<int> solution);
  bool IsBadSep(const std::vector<int>& separator, const Matrix<char>& solution_c_value) const;
};
} // namespace triangulator
