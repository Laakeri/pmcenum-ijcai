#pragma once

#include <vector>
#include <memory>

#include "sat_interface.hpp"
#include "glucose/simp/SimpSolver.h"

namespace triangulator {
class GlucoseInterface : public SatInterface {
public:
  Lit NewVar() final;
  void AddClause(std::vector<Lit> clause) final;
  bool SolutionValue(Lit lit) final;
  bool Solve(std::vector<Lit> assumptions, bool allow_simp) final;
  void FreezeVar(Lit var) final;
  GlucoseInterface(bool preprocess);
  void PrintStats(int lvl) final;
private:
  std::unique_ptr<Glucose::SimpSolver> glucose_;
  std::vector<Glucose::Var> glucose_vars_;
  int num_clauses_;
  bool preprocess_;
};
} // namespace triangulator
