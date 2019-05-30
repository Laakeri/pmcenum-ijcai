#include "minisat_interface.hpp"

#include <vector>
#include <memory>

#include "minisat/simp/SimpSolver.h"
#include "utils.hpp"

namespace triangulator {
MinisatInterface::MinisatInterface(bool preprocess)
  : minisat_(new Minisat::SimpSolver()), num_clauses_(0), preprocess_(preprocess) { }

Lit MinisatInterface::NewVar() {
  minisat_vars_.push_back(minisat_->newVar());
  return CreateLit(minisat_vars_.size());
}
void MinisatInterface::AddClause(std::vector<Lit> clause) {
  clause = SatHelper::ProcessClause(clause);
  if (clause.size() == 1 && clause[0].IsTrue()) return;
  Minisat::vec<Minisat::Lit> minisat_clause;
  for (Lit lit : clause) {
    int lit_value = LitValue(lit);
    assert(lit_value != 0 && abs(lit_value) <= minisat_vars_.size());
    Minisat::Var minisat_var = minisat_vars_[abs(lit_value) - 1];
    if (lit_value > 0) {
      minisat_clause.push(Minisat::mkLit(minisat_var, true));
    } else {
      minisat_clause.push(Minisat::mkLit(minisat_var, false));
    }
  }
  minisat_->addClause(minisat_clause);
  num_clauses_++;
}
bool MinisatInterface::SolutionValue(Lit lit) {
  assert(lit.IsDef());
  if (lit.IsTrue()) return true;
  if (lit.IsFalse()) return false;
  int lit_value = LitValue(lit);
  assert(lit_value != 0 && abs(lit_value <= minisat_vars_.size()));
  Minisat::Var minisat_var = minisat_vars_[abs(lit_value) - 1];
  bool solution_value = (Minisat::toInt(minisat_->modelValue(minisat_var)) == 1);
  if (lit_value > 0) {
    return solution_value;
  } else {
    return !solution_value;
  }
}
void MinisatInterface::FreezeVar(Lit var) {
  if (!preprocess_) return;
  int var_val = abs(LitValue(var));
  assert(var_val != 0 && var_val <= minisat_vars_.size());
  Minisat::Var minisat_var = minisat_vars_[abs(var_val) - 1];
  minisat_->setFrozen(minisat_var, true);
}
bool MinisatInterface::Solve(std::vector<Lit> assumptions, bool allow_simp) {
  if (!preprocess_) allow_simp = false;
  Minisat::vec<Minisat::Lit> minisat_assumptions;
  for (Lit lit : assumptions) {
    int lit_value = LitValue(lit);
    assert(lit_value != 0 && abs(lit_value) <= minisat_vars_.size());
    Minisat::Var minisat_var = minisat_vars_[abs(lit_value) - 1];
    if (lit_value > 0) {
      minisat_assumptions.push(Minisat::mkLit(minisat_var, true));
    } else {
      minisat_assumptions.push(Minisat::mkLit(minisat_var, false));
    }
  }
  return minisat_->solve(minisat_assumptions, allow_simp, !allow_simp);
}
void MinisatInterface::PrintStats(int lvl) {

}
} // namespace triangulator
