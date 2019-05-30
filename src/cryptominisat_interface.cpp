#include "cryptominisat_interface.hpp"

#include <vector>
#include <cassert>

#include <cryptominisat5/cryptominisat.h>

namespace triangulator {
CryptominisatInterface::CryptominisatInterface() : num_vars_(0), num_clauses_(0) { }
Lit CryptominisatInterface::NewVar() {
  solver_.new_var();
  num_vars_++;
  return CreateLit(num_vars_);
}
void CryptominisatInterface::AddClause(std::vector<Lit> clause) {
  clause = SatHelper::ProcessClause(clause);
  if (clause.size() == 1 && clause[0].IsTrue()) return;
  std::vector<CMSat::Lit> cmsat_clause;
  for (Lit lit : clause) {
    int lit_value = LitValue(lit);
    assert(lit_value != 0 && abs(lit_value) <= num_vars_);
    int cmsat_var = abs(lit_value) - 1;
    if (lit_value > 0) {
      cmsat_clause.push_back(CMSat::Lit(cmsat_var, false));
    } else {
      cmsat_clause.push_back(CMSat::Lit(cmsat_var, true));
    }
  }
  solver_.add_clause(cmsat_clause);
}
bool CryptominisatInterface::SolutionValue(Lit lit) {
  assert(lit.IsDef());
  if (lit.IsTrue()) return true;
  if (lit.IsFalse()) return false;
  int lit_value = LitValue(lit);
  assert(lit_value != 0 && abs(lit_value) <= num_vars_);
  int cmsat_var = abs(lit_value) - 1;
  bool solution_value = (solver_.get_model()[cmsat_var] == CMSat::l_True);
  if (lit_value > 0) {
    return solution_value;
  } else {
    return !solution_value;
  }
  num_clauses_++;
}
void CryptominisatInterface::FreezeVar(Lit var) {
  
}
bool CryptominisatInterface::Solve(std::vector<Lit> assumptions, bool allow_simp) {
  std::vector<CMSat::Lit> cmsat_assumptions;
  for (Lit lit : assumptions) {
    int lit_value = LitValue(lit);
    assert(lit_value != 0 && abs(lit_value) <= num_vars_);
    int cmsat_var = abs(lit_value) - 1;
    if (lit_value > 0) {
      cmsat_assumptions.push_back(CMSat::Lit(cmsat_var, false));
    } else {
      cmsat_assumptions.push_back(CMSat::Lit(cmsat_var, true));
    }
  }
  return (solver_.solve(&cmsat_assumptions) == CMSat::l_True);
}
void CryptominisatInterface::PrintStats(int lvl) {
  
}
} // namespace triangulator
