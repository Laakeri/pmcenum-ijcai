#include "ipasir_interface.hpp"

#include <vector>
#include <cassert>
#include <cstdlib>
#include <cstdio>
  
namespace triangulator {
extern "C" {
#include "ipasir.h"
}

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

IpasirInterface::IpasirInterface() : num_vars_(0), num_clauses_(0), state_(State::kInput) {
  solver_ = ipasir_init();
}
IpasirInterface::~IpasirInterface() {
  DisableStdout();
  ipasir_release(solver_);
  ReEnableStdout();
}
Lit IpasirInterface::NewVar() {
  num_vars_++;
  return CreateLit(num_vars_);
}
void IpasirInterface::AddClause(std::vector<Lit> clause) {
  clause = SatHelper::ProcessClause(clause);
  if (clause.size() == 1 && clause[0].IsTrue()) return;
  for (Lit lit : clause) {
    int lit_value = LitValue(lit);
    ipasir_add(solver_, lit_value);
  }
  ipasir_add(solver_, 0);
  num_clauses_++;
  state_ = State::kInput;
}
bool IpasirInterface::SolutionValue(Lit lit) {
  assert(state_ == State::kSat);
  assert(lit.IsDef());
  if (lit.IsTrue()) return true;
  if (lit.IsFalse()) return false;
  int lit_value = LitValue(lit);
  assert(lit_value != 0 && abs(lit_value <= num_vars_));
  bool solution_value = (ipasir_val(solver_, abs(lit_value)) == abs(lit_value));
  if (lit_value > 0) {
    return solution_value;
  } else {
    return !solution_value;
  }
}
void IpasirInterface::FreezeVar(Lit var) {
  
}
bool IpasirInterface::Solve(std::vector<Lit> assumptions, bool allow_simp) {
  DisableStdout();
  for (Lit lit : assumptions) {
    int lit_value = LitValue(lit);
    ipasir_assume(solver_, lit_value);
  }
  int status = ipasir_solve(solver_);
  ReEnableStdout();

  assert(status == 10 || status == 20);
  if (status == 10) {
    state_ = State::kSat;
    return true;
  } else {
    state_ = State::kUnsat;
    return false;
  }
}
void IpasirInterface::PrintStats(int lvl) {

}
}
