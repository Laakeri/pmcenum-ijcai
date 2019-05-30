#pragma once

#include <vector>

#include "sat_interface.hpp"

namespace triangulator {
class IpasirInterface : public SatInterface {
public:
  Lit NewVar() final;
  void AddClause(std::vector<Lit> clause) final;
  bool SolutionValue(Lit lit) final;
  bool Solve(std::vector<Lit> assumptions, bool allow_simp) final;
  void FreezeVar(Lit var) final;
  IpasirInterface();
  ~IpasirInterface();
  void PrintStats(int lvl) final;
private:
  void* solver_;
  int num_vars_, num_clauses_;
  enum class State {kInput, kSat, kUnsat};
  State state_;
};
} // namespace triangulator
