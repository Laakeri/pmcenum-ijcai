#pragma once

#include <vector>
#include <memory>

#include "sat_interface.hpp"
#include "mapleglucose/simp/SimpSolver.h"

namespace triangulator {
class MapleGlucoseInterface : public SatInterface {
public:
  Lit NewVar() final;
  void AddClause(std::vector<Lit> clause) final;
  bool SolutionValue(Lit lit) final;
  bool Solve(std::vector<Lit> assumptions, bool allow_simp) final;
  void FreezeVar(Lit var) final;
  MapleGlucoseInterface(bool preprocess);
  void PrintStats(int lvl) final;
private:
  std::unique_ptr<MapleGlucose::SimpSolver> glucose_;
  std::vector<MapleGlucose::Var> glucose_vars_;
  int num_clauses_;
  bool preprocess_;
};
} // namespace triangulator
