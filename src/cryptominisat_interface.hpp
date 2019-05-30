#pragma once

#include <vector>

#include "sat_interface.hpp"
#include "cryptominisat5/cryptominisat.h"

namespace triangulator {
class CryptominisatInterface : public SatInterface {
public:
  Lit NewVar() override;
  void AddClause(std::vector<Lit> clause) override;
  bool SolutionValue(Lit lit) override;
  bool Solve(std::vector<Lit> assumptions, bool allow_simp) override;
  void FreezeVar(Lit var) final;
  CryptominisatInterface();
  void PrintStats(int lvl) final;
private:
  CMSat::SATSolver solver_;
  int num_vars_, num_clauses_;
};
} // namespace triangulator
