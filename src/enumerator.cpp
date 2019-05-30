#include "enumerator.hpp"

#include <vector>
#include <map>
#include <memory>
#include <cassert>
#include <algorithm>
#include <random>

#include "sat_interface.hpp"
#include "graph.hpp"
#include "utils.hpp"

namespace triangulator{
// TODO: helpconstraints

void Enumerator::BuildBasicEncoding() {
  // Selection variables
  for (int i = 0; i < graph_.n(); i++) {
    x_var_[i] = solver_->NewVar();
    solver_->FreezeVar(x_var_[i]);
  }
  // Connectivity variables
  for (int i = 0; i < graph_.n(); i++) {
    for (int ii = 0; ii < graph_.n(); ii++) {
      if (i == ii) continue;
      if (i < ii) {
        if (graph_.HasEdge(i, ii)) c_var_[i][ii] = Lit::TrueLit();
        else c_var_[i][ii] = solver_->NewVar();
      }
      else {
        assert(c_var_[ii][i].IsDef());
        c_var_[i][ii] = c_var_[ii][i];
      }
    }
  }
  // Basic connectivity implications
  for (int i = 0; i < graph_.n(); i++) {
    for (int ii = 0; ii < graph_.n(); ii++) {
      if (i == ii) continue;
      for (int iii : graph_.Neighbors(ii)) {
        if (iii != i) solver_->AddClause({-c_var_[i][ii], x_var_[ii], c_var_[i][iii]});
      }
    }
  }
  // Check that pmc will become a clique
  for (int i = 0; i < graph_.n(); i++) {
    for (int ii = i + 1; ii < graph_.n(); ii++) {
      solver_->AddClause({-x_var_[i], -x_var_[ii], c_var_[i][ii]});
    }
  }
  // Check that there is no full component
  for (int i = 0; i < graph_.n(); i++) {
    std::vector<Lit> new_clause = {x_var_[i]};
    for (int ii = 0; ii < graph_.n(); ii++) {
      if (i == ii) continue;
      Lit new_var = solver_->NewVar();
      solver_->AddClause({x_var_[ii], -new_var});
      solver_->AddClause({-c_var_[i][ii], -new_var});
      if (minsep_encoding_ == 5) {
        solver_->AddClause({-x_var_[ii], c_var_[i][ii], new_var});
      }

      new_clause.push_back(new_var);
    }
    solver_->AddClause(new_clause);
  }  
}

void Enumerator::BuildPathLengthEncoding() {
  std::vector<Matrix<Lit>> cl_var(graph_.n(), Matrix<Lit>(graph_.n(), graph_.n()));
  std::vector<Matrix<Lit>> f_var(graph_.n(), Matrix<Lit>(graph_.n(), graph_.n()));
  for (int j = 1; j < graph_.n(); j++) {
    for (int i = 0; i < graph_.n(); i++) {
      for (int ii = 0; ii < graph_.n(); ii++) {
        if (i == ii) continue;
        if (i < ii) {
          if (graph_.HasEdge(i, ii)) {
            cl_var[j][i][ii] = Lit::TrueLit();
          } else if (j == 1) {
            cl_var[j][i][ii] = Lit::FalseLit();
          } else {
            cl_var[j][i][ii] = solver_->NewVar();
          }
        }
        else {
          assert(cl_var[j][ii][i].IsDef());
          cl_var[j][i][ii] = cl_var[j][ii][i];
        }
        f_var[j][i][ii] = solver_->NewVar();
        solver_->AddClause({-f_var[j][i][ii], cl_var[j][i][ii]});
        solver_->AddClause({-f_var[j][i][ii], -x_var_[ii]});
      }
    }
  }
  for (int j = 1; j + 1 < graph_.n(); j++) {
    for (int i = 0; i < graph_.n(); i++) {
      for (int ii = 0; ii < graph_.n(); ii++) {
        if (i == ii) continue;
        if (i < ii) {
          solver_->AddClause({-cl_var[j][i][ii], cl_var[j+1][i][ii]});
        }
        solver_->AddClause({-f_var[j][i][ii], f_var[j+1][i][ii]});
      }
    }
  }
  for (int j = 2; j < graph_.n(); j++) {
    for (int i = 0; i < graph_.n(); i++) {
      for (int ii = 0; ii < graph_.n(); ii++) {
        if (i == ii) continue;
        if (graph_.HasEdge(i, ii)) continue;
        std::vector<Lit> new_clause = {-cl_var[j][i][ii]};
        for (int iii : graph_.Neighbors(ii)) {
          if (iii == i) continue;
          new_clause.push_back({f_var[j-1][i][iii]});
        }
        solver_->AddClause(new_clause);
      }
    }
  }
  for (int i = 0; i < graph_.n(); i++) {
    for (int ii = i + 1; ii < graph_.n(); ii++) {
      solver_->AddClause({-c_var_[i][ii], cl_var[graph_.n()-1][i][ii]});
    }
  }
}

Enumerator::Enumerator(const Graph& graph, std::shared_ptr<SatInterface> solver, int minsep_encoding)
  : x_var_(graph.n()), c_var_(graph.n(), graph.n()), solver_(solver), graph_(graph), minsep_encoding_(minsep_encoding) {
  BuildBasicEncoding();
  if (minsep_encoding_ == 4) {
    BuildPathLengthEncoding();
  }
}

bool Enumerator::IsBadSep(const std::vector<int>& separator, const Matrix<char>& solution_c_value) const {
  auto connectedmatrix = graph_.ConnectedMatrix(separator);
  for (int i = 0; i < graph_.n(); i++) {
    for (int ii = 0; ii < graph_.n(); ii++) {
      if (i != ii && solution_c_value[i][ii] && !connectedmatrix[i][ii]) return true;
    }
  }
  return false;
}

void Enumerator::BlockBadSolution(std::vector<int> solution) {
  std::sort(solution.begin(), solution.end());
  std::vector<int> minsep = solution;
  Log::Write(30, "Minimizing sep of size ", minsep.size());
  Matrix<char> solution_c_value(graph_.n(), graph_.n());
  for (int i = 0; i < graph_.n(); i++) {
    for (int ii = 0; ii < graph_.n(); ii++) {
      if (i != ii) solution_c_value[i][ii] = solver_->SolutionValue(c_var_[i][ii]);
    }
  }
  assert(IsBadSep(minsep, solution_c_value));
  for (int i = 0; i < minsep.size(); i++) {
    std::vector<int> new_minsep;
    for (int ii = 0; ii < minsep.size(); ii++) {
      if (ii != i) new_minsep.push_back(minsep[ii]);
    }
    if (IsBadSep(new_minsep, solution_c_value)) {
      minsep = new_minsep;
      i--;
    }
  }
  Log::Write(20, "Found minsep of size ", minsep.size());
  std::vector<std::vector<int> > components = graph_.Components(minsep);
  assert(components.size() >= 2);
  int vars_added = 0;
  int clauses_added = 0;
  Lit minsep_var;
  assert(utils::IsSorted(minsep));
  if (minsep_vars_.count(minsep)) {
    Log::Write(30, "Minsep already known");
    minsep_var = minsep_vars_[minsep];
  }
  else {
    Log::Write(30, "New minsep");
    enumerator_stats_.MinsepFound();
    minsep_var = solver_->NewVar();
    if (minsep_encoding_ != 0 && minsep_encoding_ != 5) {
      // In some encodings we might refer the variable again, so it should be frozen.
      solver_->FreezeVar(minsep_var);
    }
    minsep_vars_[minsep] = minsep_var;
    vars_added++;
    std::vector<Lit> minsep_clause = {minsep_var};
    for (int v : minsep) {
      minsep_clause.push_back(-x_var_[v]);
    }
    solver_->AddClause(minsep_clause);
    clauses_added++;
    if (minsep_encoding_ == 5) {
      for (int v : minsep) {
        solver_->AddClause({-minsep_var, x_var_[v]});
      }
    }
  }
  switch (minsep_encoding_) {
    case 0:
    case 5:
    {
      // Add all pairs of vertices
      for (int i = 0; i < components.size(); i++) {
        for (int ii = i + 1; ii < components.size(); ii++) {
          for (int v : components[i]) {
            for (int u : components[ii]) {
              solver_->AddClause({-minsep_var, -c_var_[v][u]});
              clauses_added++;
            }
          }
        }
      }
      break;
    }
    case 1: {
      // Add the pairs that are broken in the current solution
      for (int i = 0; i < components.size(); i++) {
        for (int ii = i + 1; ii < components.size(); ii++) {
          for (int v : components[i]) {
            for (int u : components[ii]) {
              if (solution_c_value[v][u]) {
                solver_->AddClause({-minsep_var, -c_var_[v][u]});
                clauses_added++;
              }
            }
          }
        }
      }
      break;
    }
    case 2: {
      // Add one random pair for each pair of components that is broken
      for (int i = 0; i < components.size(); i++) {
        for (int ii = i+1; ii < components.size(); ii++) {
          bool found = false;
          std::shuffle(components[i].begin(), components[i].end(), random_gen_);
          std::shuffle(components[ii].begin(), components[ii].end(), random_gen_);
          for (int v : components[i]) {
            for (int u : components[ii]) {
              if (solution_c_value[v][u]) {
                solver_->AddClause({-minsep_var, -c_var_[v][u]});
                clauses_added++;
                found = true;
                break;
              }
            }
            if (found) break;
          }
        }
      }
      break;
    }
    case 3: {
      // If a pair of components is broken add all pairs of vertices between them
      for (int i = 0; i < components.size(); i++) {
        for (int ii = i + 1; ii < components.size(); ii++) {
          bool broken = false;
          for (int v : components[i]) {
            for (int u : components[ii]) {
              if (solution_c_value[v][u]) {
                broken = true;
                break;
              }
            }
            if (broken) break;
          }
          if (broken) {
            for (int v : components[i]) {
              for (int u : components[ii]) {
                solver_->AddClause({-minsep_var, -c_var_[v][u]});
                clauses_added++;
              }
            }
          }
        }
      }
      break;
    }
    default: {
      assert(false);
    }
  }
  Log::Write(30, "Variables added ", vars_added);
  Log::Write(30, "Clauses added ", clauses_added);
}

std::vector<int> Enumerator::GetPmc(std::vector<Lit> assumptions, bool first_call) {
  while (true) {
    enumerator_stats_.SatCalled();
    bool sat = solver_->Solve(assumptions, first_call);
    if (!sat) return {};
    std::vector<Lit> block_clause;
    std::vector<int> solution;
    for (int i = 0; i < graph_.n(); i++) {
      if (solver_->SolutionValue(x_var_[i])) {
        block_clause.push_back(-x_var_[i]);
        solution.push_back(i);
      }
      else {
        // This is not needed if the problem specific encoding asserts the size of the PMC.
        // But in general it is needed.
        block_clause.push_back(x_var_[i]);
      }
    }
    if (graph_.IsPmc(solution)) {
      enumerator_stats_.PmcFound();
      Log::Write(20, "Found pmc of size ", solution.size());
      solver_->AddClause(block_clause);
      return solution;
    }
    else {
      BlockBadSolution(solution);
    }
  }
}

const std::vector<Lit>& Enumerator::XVars() const {
  return x_var_;
}
void Enumerator::PrintStats(int lvl) const {
  enumerator_stats_.Print(lvl);
  solver_->PrintStats(lvl);
}
EnumeratorStats Enumerator::Stats() const {
  return enumerator_stats_;
}
EnumeratorStats::EnumeratorStats() : pmcs_found_(0), minseps_found_(0), sat_calls_(0) { }
void EnumeratorStats::PmcFound() {
  pmcs_found_++;
}
void EnumeratorStats::MinsepFound() {
  minseps_found_++;
}
void EnumeratorStats::SatCalled() {
  sat_calls_++;
}
int EnumeratorStats::PmcsFound() const {
  return pmcs_found_;
}
int EnumeratorStats::MinsepsFound() const {
  return minseps_found_;
}
int EnumeratorStats::SatCalls() const {
  return sat_calls_;
}
void EnumeratorStats::Print(int lvl) const {
  Log::Write(lvl, "Enumerator stats:");
  Log::Write(lvl, "PMCs found: ", pmcs_found_);
  Log::Write(lvl, "Minseps found: ", minseps_found_);
  Log::Write(lvl, "Sat calls: ", sat_calls_);
}
} // namespace triangulator
