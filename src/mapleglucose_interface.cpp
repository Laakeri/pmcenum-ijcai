#include "mapleglucose_interface.hpp"

#include <vector>
#include <memory>
#include <cstdlib>
#include <cstdio>

#include "mapleglucose/simp/SimpSolver.h"
#include "utils.hpp"

namespace {
// Macros for disabling stdout.
#ifdef _WIN32
#include <io.h>
char * const nulFileName = "NUL";
#define CROSS_DUP(fd) _dup(fd)
#define CROSS_DUP2(fd, newfd) _dup2(fd, newfd)
#else
#include <unistd.h>
char * const nulFileName = "/dev/null";
#define CROSS_DUP(fd) dup(fd)
#define CROSS_DUP2(fd, newfd) dup2(fd, newfd)
#endif
  
// Functions for disabling stdout. Source: https://stackoverflow.com/questions/13498169/c-how-to-suppress-a-sub-functions-output.
int stdoutBackupFd; 
FILE *nullOut;  
void DisableStdout() {
  stdoutBackupFd = CROSS_DUP(STDOUT_FILENO);

  fflush(stdout);
  nullOut = fopen(nulFileName, "w");
  CROSS_DUP2(fileno(nullOut), STDOUT_FILENO);
}
void ReEnableStdout() {
  fflush(stdout);
  fclose(nullOut);
  CROSS_DUP2(stdoutBackupFd, STDOUT_FILENO);
  close(stdoutBackupFd);
}
} // namespace

namespace triangulator {
MapleGlucoseInterface::MapleGlucoseInterface(bool preprocess)
  : glucose_(new MapleGlucose::SimpSolver()), num_clauses_(0), preprocess_(preprocess) { }

Lit MapleGlucoseInterface::NewVar() {
  glucose_vars_.push_back(glucose_->newVar());
  return CreateLit(glucose_vars_.size());
}
void MapleGlucoseInterface::AddClause(std::vector<Lit> clause) {
  clause = SatHelper::ProcessClause(clause);
  if (clause.size() == 1 && clause[0].IsTrue()) return;
  MapleGlucose::vec<MapleGlucose::Lit> glucose_clause;
  for (Lit lit : clause) {
    int lit_value = LitValue(lit);
    assert(lit_value != 0 && abs(lit_value) <= glucose_vars_.size());
    MapleGlucose::Var glucose_var = glucose_vars_[abs(lit_value) - 1];
    if (lit_value > 0) {
      glucose_clause.push(MapleGlucose::mkLit(glucose_var, true));
    } else {
      glucose_clause.push(MapleGlucose::mkLit(glucose_var, false));
    }
  }
  glucose_->addClause(glucose_clause);
  num_clauses_++;
}
bool MapleGlucoseInterface::SolutionValue(Lit lit) {
  assert(lit.IsDef());
  if (lit.IsTrue()) return true;
  if (lit.IsFalse()) return false;
  int lit_value = LitValue(lit);
  assert(lit_value != 0 && abs(lit_value <= glucose_vars_.size()));
  MapleGlucose::Var glucose_var = glucose_vars_[abs(lit_value) - 1];
  bool solution_value = (MapleGlucose::toInt(glucose_->modelValue(glucose_var)) == 1);
  if (lit_value > 0) {
    return solution_value;
  } else {
    return !solution_value;
  }
}
void MapleGlucoseInterface::FreezeVar(Lit var) {
  if (!preprocess_) return;
  int var_val = abs(LitValue(var));
  assert(var_val != 0 && var_val <= glucose_vars_.size());
  MapleGlucose::Var glucose_var = glucose_vars_[abs(var_val) - 1];
  glucose_->setFrozen(glucose_var, true);
}
bool MapleGlucoseInterface::Solve(std::vector<Lit> assumptions, bool allow_simp) {
  if (!preprocess_) allow_simp = false;
  MapleGlucose::vec<MapleGlucose::Lit> glucose_assumptions;
  for (Lit lit : assumptions) {
    int lit_value = LitValue(lit);
    assert(lit_value != 0 && abs(lit_value) <= glucose_vars_.size());
    MapleGlucose::Var glucose_var = glucose_vars_[abs(lit_value) - 1];
    if (lit_value > 0) {
      glucose_assumptions.push(MapleGlucose::mkLit(glucose_var, true));
    } else {
      glucose_assumptions.push(MapleGlucose::mkLit(glucose_var, false));
    }
  }
  DisableStdout();
  bool status = glucose_->solve(glucose_assumptions, allow_simp, !allow_simp);
  ReEnableStdout();
  return status;
}
void MapleGlucoseInterface::PrintStats(int lvl) {

}
} // namespace triangulator
